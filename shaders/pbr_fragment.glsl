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
uniform vec3  viewPos;
uniform vec3  lightDir;
uniform vec3  lightColor;
uniform float lightEnergy;  // Godot's Light3D.light_energy; 1.0 = baseline.
                            // Defaults to 5.0 so our PBR (albedo/PI scaled)
                            // actually reads as "lit" without a visible-only
                            // energy ramp in post.
uniform float exposure;
uniform mat4  view;         // needed for view-space depth in cascade pick

// Cascaded shadow maps (directional light). Array layer per cascade.
uniform sampler2DArray cascadeShadowMap;
uniform mat4   lightSpaceMatrices[4];
uniform float  cascadeSplits[4];   // view-space far plane per cascade
uniform int    numCascades;        // 4 in practice
uniform float  shadowBias;
uniform float  shadowNormalBias;
// PCSS soft-shadow controls (Phase I). shadowSoftness = Godot's
// "light angular size" — scales the blocker search radius and the
// final PCF kernel. 0 = hard shadows (skip blocker search).
// shadowQuality = 0 low (cheap 4-tap), 1 high (16-tap).
uniform float  shadowSoftness;
uniform int    shadowQuality;

// SSAO (Phase 4)
uniform sampler2D ssaoTexture;
uniform bool useSSAO;

// --- Clustered point/spot lights (Phase B lighting migration) ---
//
// LightManager owns the three SSBOs populated each frame by the
// cluster-cull compute shader. Layout must match `struct Light` in
// include/Light.h *and* `shaders/cluster_cull.comp`: four vec4s per
// light (64 bytes, std430-aligned).
struct GPULight {
    vec4 position;   // xyz = world position, w = type (0=dir, 1=point, 2=spot)
    vec4 direction;  // xyz = world direction, w = inner cone cos
    vec4 color;      // rgb = color, w = per-light energy (unused for now)
    vec4 params;     // x = range, y = outer cone cos, z = shadow index, w = _
};
layout(std430, binding = 2) readonly buffer LightBuffer  { GPULight gpuLights[]; };
layout(std430, binding = 3) readonly buffer LightIndices { int lightIndices[]; };
layout(std430, binding = 4) readonly buffer LightGrid    { ivec2 lightGrid[]; };

uniform vec2  screenSize;    // pixels, for cluster-x/y lookup
uniform float nearPlane;
uniform float farPlane;
uniform bool  useClusteredLights;

// Omni shadow atlas — cubemap-array, one cube per shadowing light.
// Light's `params.z` picks the layer; sampling direction is
// (fragPos - lightPos) normalized. Shader compares the raw distance
// (normalized by omniShadowFar) against the atlas depth written by
// the omni depth pass.
uniform samplerCubeArray omniShadowMaps;
uniform float            omniShadowFar;

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

// --- Clustered light helpers ---
//
// Cluster grid is 16x9x24 in screen × log-depth, matching
// LightManager's constants. Depth slicing is logarithmic so the
// math inverts log(viewZ/near) / log(far/near).
const int  CLUSTER_X = 16;
const int  CLUSTER_Y = 9;
const int  CLUSTER_Z = 24;

int clusterIndex(vec2 fragCoord, float viewDepth) {
    vec2 tileSize = screenSize / vec2(float(CLUSTER_X), float(CLUSTER_Y));
    int cx = int(fragCoord.x / tileSize.x);
    int cy = int(fragCoord.y / tileSize.y);
    float zNorm = log(max(viewDepth, nearPlane) / nearPlane)
                / log(farPlane / nearPlane);
    int cz = int(zNorm * float(CLUSTER_Z));
    cx = clamp(cx, 0, CLUSTER_X - 1);
    cy = clamp(cy, 0, CLUSTER_Y - 1);
    cz = clamp(cz, 0, CLUSTER_Z - 1);
    return cx + cy * CLUSTER_X + cz * (CLUSTER_X * CLUSTER_Y);
}

// Godot-style smooth range attenuation. Smooth-drops to 0 at range,
// physically-ish inverse-square inside. Keeps the cluster cull tight
// without a hard boundary-popping artifact.
float pointAttenuation(float dist, float range) {
    if (range <= 0.0) return 0.0;
    float invSq = 1.0 / max(dist * dist, 1e-4);
    float falloff = clamp(1.0 - pow(dist / range, 4.0), 0.0, 1.0);
    falloff *= falloff;
    return invSq * falloff;
}

// Returns the spot cone mask [0..1]. innerCos/outerCos come from the
// light struct (already cos(angle) on the CPU side).
float spotCone(vec3 L, vec3 spotDir, float innerCos, float outerCos) {
    float theta = dot(-L, normalize(spotDir));
    return clamp((theta - outerCos) / max(innerCos - outerCos, 1e-4), 0.0, 1.0);
}

// Returns the per-light contribution using the same Schlick+GGX+Lambert
// math as the directional path, parameterized by an incoming light dir
// and pre-attenuated color.
vec3 shadeLight(vec3 N, vec3 V, vec3 L, vec3 lightColor_,
                vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3  H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = FresnelSchlick(max(dot(H, V), 0.0), F0);
    vec3  numerator    = NDF * G * F;
    float denominator  = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-4;
    vec3  specular     = numerator / denominator;
    vec3  kS = F;
    vec3  kD = (vec3(1.0) - kS) * (1.0 - metallic);
    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * lightColor_ * NdotL;
}

// --- CSM Shadow Calculation ---
//
// Picks one of the 4 cascades by view-space depth, then samples the
// sampler2DArray at that layer. Slope-scaled + normal-biased as Godot
// does. Blends across cascade boundaries via smoothstep to hide the
// seam.
int pickCascade(float viewDepth) {
    for (int i = 0; i < 4; ++i) {
        if (viewDepth < cascadeSplits[i]) return i;
    }
    return 3;
}

// Vogel disk — golden-angle spiral for even sample coverage. Used
// by both the blocker search and the penumbra PCF.
vec2 vogelDisk(int i, int n, float phi) {
    float r = sqrt((float(i) + 0.5) / float(n));
    float theta = float(i) * 2.39996322972865332 + phi;
    return vec2(cos(theta), sin(theta)) * r;
}

// Per-pixel random phi so the Vogel disk doesn't tile visibly.
// Under TAA this resolves as a clean penumbra; without TAA there's
// slight noise but still smoother than a fixed pattern.
float rand01(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

float sampleCascade(int layer, vec3 worldPos, vec3 N, vec3 L) {
    vec4 lsPos = lightSpaceMatrices[layer] * vec4(worldPos, 1.0);
    vec3 proj  = lsPos.xyz / lsPos.w;
    proj       = proj * 0.5 + 0.5;
    if (proj.z > 1.0)          return 0.0;   // behind far plane -> unshadowed
    if (any(lessThan(proj.xy, vec2(0.0))) ||
        any(greaterThan(proj.xy, vec2(1.0)))) return 0.0;   // outside cascade

    // Cascade-aware bias. Farther cascades cover larger frustums, so
    // a single bias is too small for cascade 3 and too big for
    // cascade 0. Scale by (layer+1) as a cheap first-pass; Godot
    // uses per-cascade `shadow_bias[4]` uniforms — covered by
    // per-light shadowBias from LightComponent now.
    float ndotl       = max(dot(N, L), 0.0);
    float cascadeMult = float(layer + 1);
    float bias        = max(shadowBias * cascadeMult * (1.0 - ndotl),
                            0.002 * cascadeMult) * shadowNormalBias;

    vec2 texel = 1.0 / vec2(textureSize(cascadeShadowMap, 0).xy);

    // Hard shadow fast path — softness=0 skips the PCF kernel.
    if (shadowSoftness <= 0.001) {
        float pcfDepth = texture(
            cascadeShadowMap,
            vec3(proj.xy, float(layer))).r;
        return proj.z - bias > pcfDepth ? 1.0 : 0.0;
    }

    // --- Proper PCSS with separated blocker bias ---
    //
    // Key stability fix vs. the previous attempt: the blocker search
    // uses a noticeably LARGER bias than the final shadow compare
    // (`blockerBias = bias * 3`). This prevents the receiver
    // fragment's own depth from qualifying as a blocker when it
    // happens to land on a neighbouring texel with nearly-identical
    // depth — the exact failure mode that was leaving pillar tops
    // fully shadowed.
    int   blockerTaps = (shadowQuality == 1) ? 8  : 4;
    int   pcfTaps     = (shadowQuality == 1) ? 32 : 16;
    float phi         = rand01(gl_FragCoord.xy) * 6.2831853;

    float searchRadius = (1.0 + shadowSoftness * 3.0) * texel.x;
    float blockerBias  = bias * 3.0;

    // Blocker search — average depth of samples that are REAL
    // occluders (meaningfully closer to the light than this fragment).
    float avgBlocker   = 0.0;
    int   blockerCount = 0;
    for (int i = 0; i < blockerTaps; ++i) {
        vec2 off = vogelDisk(i, blockerTaps, phi) * searchRadius;
        float d  = texture(cascadeShadowMap,
                            vec3(proj.xy + off, float(layer))).r;
        if (d < proj.z - blockerBias) {
            avgBlocker += d;
            blockerCount++;
        }
    }
    if (blockerCount == 0) return 0.0;   // no real blocker -> fully lit
    avgBlocker /= float(blockerCount);

    // Penumbra estimate: proportional to (receiver - blocker) gap
    // scaled by softness. Clamped so a single anomalous blocker
    // can't blow the kernel radius out.
    float depthDiff = max(proj.z - avgBlocker, 0.0);
    float penumbra  = depthDiff * shadowSoftness * 15.0;
    float pcfRadius = clamp((1.0 + penumbra) * texel.x,
                            texel.x,         // min: 1-texel
                            texel.x * 12.0); // max: 12-texel kernel

    // Main PCF — use the original bias (not blocker bias) so the
    // penumbra edge is consistent with the hard shadow path at
    // softness→0.
    float shadow = 0.0;
    for (int i = 0; i < pcfTaps; ++i) {
        vec2 off = vogelDisk(i, pcfTaps, phi) * pcfRadius;
        float d  = texture(cascadeShadowMap,
                            vec3(proj.xy + off, float(layer))).r;
        shadow += proj.z - bias > d ? 1.0 : 0.0;
    }
    return shadow / float(pcfTaps);
}

float ShadowCalculationCSM(vec3 worldPos, vec3 N, vec3 L) {
    // View-space Z is negative of (viewMatrix * worldPos).z for a
    // right-handed view — use the absolute magnitude for the cascade
    // split comparison. view uniform is the main camera view matrix.
    float viewDepth = -(view * vec4(worldPos, 1.0)).z;
    int layer = pickCascade(viewDepth);
    float s0 = sampleCascade(layer, worldPos, N, L);

    // Blend with next cascade near the boundary to hide seams.
    if (layer < 3) {
        float splitWidth = (layer == 0)
            ? cascadeSplits[0]
            : cascadeSplits[layer] - cascadeSplits[layer - 1];
        float blend = smoothstep(cascadeSplits[layer] - splitWidth * 0.15,
                                 cascadeSplits[layer],
                                 viewDepth);
        if (blend > 0.0) {
            float s1 = sampleCascade(layer + 1, worldPos, N, L);
            s0 = mix(s0, s1, blend);
        }
    }
    return s0;
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

    // Previously: `if (!gl_FrontFacing) N = -N;` for "two-sided
    // lighting". That flip was wrong for flat geometry — the plane
    // mesh's back face got its normal inverted to point DOWN, so the
    // ground's NdotL against the (downward) sun went negative and it
    // rendered as pure ambient (≈ black). Removed; proper
    // double-sided support lands as a per-material flag later.

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
    // lightEnergy scales the direct contribution — Godot's Light3D
    // ships 1.0 as baseline but our tonemap + AgX expect ~5 for
    // perceptually "lit" scenes without auto-exposure.
    vec3 Lo = (kD * albedo / PI + specular) * lightColor * lightEnergy * NdotL;

    // Shadow — CSM sampled from the 4-layer array. The old 2D
    // shadowMap sampler is gone; this is the only directional
    // shadow path now.
    float shadow = ShadowCalculationCSM(fs_in.FragPos, N, L);

    // --- Clustered point/spot lights ---
    //
    // Look up this fragment's cluster cell (screen-space XY + log
    // depth Z) in lightGrid, then iterate `count` entries in
    // lightIndices starting at `offset`. Skip type=0 (directional)
    // entries — the legacy `lightDir`/`lightColor` uniforms handle
    // the sun until ECS integration lands.
    if (useClusteredLights) {
        float viewZ = -(view * vec4(fs_in.FragPos, 1.0)).z;
        int cIdx = clusterIndex(gl_FragCoord.xy, viewZ);
        ivec2 grid = lightGrid[cIdx];
        int offset = grid.x;
        int count  = grid.y;
        for (int i = 0; i < count; ++i) {
            int idx = lightIndices[offset + i];
            GPULight gl = gpuLights[idx];
            int type = int(gl.position.w);
            if (type == 0) continue;               // directional handled above

            vec3  lightPos = gl.position.xyz;
            vec3  toLight  = lightPos - fs_in.FragPos;
            float dist     = length(toLight);
            vec3  Ld       = toLight / max(dist, 1e-4);
            float range    = gl.params.x;
            float att      = pointAttenuation(dist, range);
            if (att <= 0.0) continue;

            if (type == 2) {
                // Spot light: multiply by cone mask.
                float innerCos = gl.direction.w;
                float outerCos = gl.params.y;
                att *= spotCone(Ld, gl.direction.xyz, innerCos, outerCos);
                if (att <= 0.0) continue;
            }

            // Omni shadow lookup — only for point lights with a
            // valid atlas slot. `gl.params.z` carries the layer
            // index (-1 = no shadow, just skip). Sample direction
            // points from light toward the fragment.
            float lightShadow = 0.0;
            if (type == 1 && gl.params.z >= 0.0) {
                int   slot  = int(gl.params.z);
                vec3  dirFromLight = fs_in.FragPos - lightPos;
                float refDist = length(dirFromLight);
                // Normalize by the light's range (matches omni depth
                // pass's farPlane = light range).
                float refNorm = refDist / max(range, 0.001);
                float bias = 0.005;
                // textureLod(sampCubeArray, vec4(dir, layer), 0)
                float stored = texture(omniShadowMaps,
                                       vec4(normalize(dirFromLight), float(slot))).r;
                lightShadow = (refNorm - bias > stored) ? 1.0 : 0.0;
            }

            // Light energy lives in gl.color.w (intensity multiplier).
            vec3 lc = gl.color.rgb * gl.color.w * att;
            Lo += (1.0 - lightShadow)
                * shadeLight(N, V, Ld, lc, albedo, metallic, roughness, F0);
        }
    }

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
