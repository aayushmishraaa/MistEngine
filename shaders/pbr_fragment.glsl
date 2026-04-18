#version 460 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec4 FragPosLightSpace;
    mat3 TBN;
} fs_in;

// Material uniforms
struct Material {
    sampler2D albedoMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D aoMap;
    sampler2D emissiveMap;

    bool hasAlbedoMap;
    bool hasNormalMap;
    bool hasMetallicMap;
    bool hasRoughnessMap;
    bool hasAOMap;
    bool hasEmissiveMap;

    vec3  albedoColor;
    float metallicValue;
    float roughnessValue;
    float aoValue;
    vec3  emissiveColor;
};
uniform Material material;

// Lighting
uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float exposure;

// Shadows
uniform sampler2D shadowMap;

// SSAO (Phase 4)
uniform sampler2D ssaoTexture;
uniform bool useSSAO;

// IBL (Phase 7)
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform bool useIBL;

const float PI = 3.14159265359;

// --- PBR Functions ---

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// --- Shadow Calculation ---

float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDirection) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // Slope-scaled bias
    float bias = max(0.005 * (1.0 - dot(normal, lightDirection)), 0.001);

    // PCF (4x4 kernel)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -2; x <= 2; ++x) {
        for (int y = -2; y <= 2; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 25.0;

    return shadow;
}

void main() {
    // Sample material properties
    vec3 albedo = material.hasAlbedoMap ?
        pow(texture(material.albedoMap, fs_in.TexCoords).rgb, vec3(2.2)) :
        material.albedoColor;

    float metallic = material.hasMetallicMap ?
        texture(material.metallicMap, fs_in.TexCoords).r :
        material.metallicValue;

    float roughness = material.hasRoughnessMap ?
        texture(material.roughnessMap, fs_in.TexCoords).r :
        material.roughnessValue;

    float ao = material.hasAOMap ?
        texture(material.aoMap, fs_in.TexCoords).r :
        material.aoValue;

    vec3 emissive = material.hasEmissiveMap ?
        pow(texture(material.emissiveMap, fs_in.TexCoords).rgb, vec3(2.2)) :
        material.emissiveColor;

    // Normal mapping
    vec3 N;
    if (material.hasNormalMap) {
        vec3 tangentNormal = texture(material.normalMap, fs_in.TexCoords).xyz * 2.0 - 1.0;
        N = normalize(fs_in.TBN * tangentNormal);
    } else {
        N = normalize(fs_in.Normal);
    }

    // Two-sided lighting: flip normal for back faces (e.g. inside room walls)
    if (!gl_FrontFacing) N = -N;

    vec3 V = normalize(viewPos - fs_in.FragPos);
    vec3 R = reflect(-V, N);

    // F0 for Fresnel
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // Apply SSAO
    if (useSSAO) {
        float ssaoVal = texture(ssaoTexture, gl_FragCoord.xy / textureSize(ssaoTexture, 0)).r;
        ao *= ssaoVal;
    }

    // --- Direct Lighting (single directional light for now) ---
    vec3 L = normalize(-lightDir);
    vec3 H = normalize(V + L);

    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    // H = normalize(V + L), so dot(H, V) == dot(H, L); both are valid Schlick inputs.
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular     = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    float NdotL = max(dot(N, L), 0.0);
    vec3 Lo = (kD * albedo / PI + specular) * lightColor * NdotL;

    // Shadow
    float shadow = ShadowCalculation(fs_in.FragPosLightSpace, N, L);

    // Ambient - IBL or constant
    vec3 ambient;
    if (useIBL) {
        vec3 kSamb = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
        vec3 kDamb = 1.0 - kSamb;
        kDamb *= 1.0 - metallic;

        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo;

        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf = texture(brdfLUT, vec2(max(dot(N, V), 0.0), roughness)).rg;
        vec3 specularIBL = prefilteredColor * (kSamb * brdf.x + brdf.y);

        ambient = (kDamb * diffuseIBL + specularIBL) * ao;
    } else {
        ambient = vec3(0.15) * albedo * ao;
    }

    vec3 color = ambient + (1.0 - shadow) * Lo + emissive;

    // Output linear HDR (tone mapping done in post-process)
    FragColor = vec4(color, 1.0);
}
