#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

uniform sampler2D diffuseTexture;
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
    
    // Check if we're outside the shadow map
    if (projCoords.z > 1.0) {
        return 0.0;
    }
    
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    
    // Simple shadow bias to reduce shadow acne
    float bias = 0.005;
    float shadow = currentDepth - bias > closestDepth ? 1.0 : 0.0;
    
    return shadow;
}

void main() {
    // Sample the diffuse texture
    vec4 textureColor = texture(diffuseTexture, TexCoords);
    vec3 color = textureColor.rgb;
    
    // If texture sampling failed or returned very dark colors, use a default color
    if (length(color) < 0.1) {
        color = vec3(0.8, 0.8, 0.8); // Light gray default
    }
    
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDirNorm = normalize(-lightDir);
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.2;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    // Orb lighting (if orb exists)
    vec3 orbLighting = vec3(0.0);
    if (length(orbPosition) > 0.1) {
        vec3 orbDir = normalize(orbPosition - FragPos);
        float orbDistance = length(orbPosition - FragPos);
        float orbAttenuation = 1.0 / (1.0 + 0.1 * orbDistance + 0.01 * orbDistance * orbDistance);
        vec3 orbDiffuse = max(dot(norm, orbDir), 0.0) * orbColor * orbAttenuation * 2.0;
        vec3 orbAmbient = orbColor * orbAttenuation * 0.2;
        orbLighting = orbDiffuse + orbAmbient;
    }
    
    // Shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    vec3 lighting = (ambient + orbLighting + (1.0 - shadow) * (diffuse + specular)) * color;
    
    FragColor = vec4(lighting, textureColor.a);
}