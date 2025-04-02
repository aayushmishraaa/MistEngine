#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D shadowMap;

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 orbPosition;
uniform vec3 orbColor;

float ShadowCalculation(vec4 fragPosLightSpace) {
    // Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth > closestDepth ? 1.0 : 0.0;
    return shadow;
}

void main() {
    // Sample textures
    vec3 diffuseColor = texture(texture_diffuse1, TexCoords).rgb;
    vec3 specularColor = texture(texture_specular1, TexCoords).rgb;
    vec3 normalMap = texture(texture_normal1, TexCoords).rgb;
    normalMap = normalize(normalMap * 2.0 - 1.0); // Transform from [0,1] to [-1,1]

    // Ambient
    float ambientStrength = 0.5;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * specularColor;

    // Orb lighting
    vec3 orbDir = normalize(orbPosition - FragPos);
    float orbDistance = length(orbPosition - FragPos);
    float orbAttenuation = 1.0 / (1.0 + 0.1 * orbDistance + 0.01 * orbDistance * orbDistance);
    vec3 orbDiffuse = max(dot(norm, orbDir), 0.0) * orbColor * orbAttenuation * 2.0;
    vec3 orbAmbient = orbColor * orbAttenuation * 0.2;

    // Shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    vec3 lighting = (ambient + orbAmbient + (1.0 - shadow) * (diffuse + specular + orbDiffuse)) * diffuseColor;

    FragColor = vec4(lighting, 1.0);
}
