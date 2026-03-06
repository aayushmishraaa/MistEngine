#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
layout (location = 5) in ivec4 aBoneIDs;
layout (location = 6) in vec4 aBoneWeights;

layout(std430, binding = 6) buffer BoneMatrices {
    mat4 boneTransforms[];
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 Normal;
    vec4 FragPosLightSpace;
    mat3 TBN;
} vs_out;

void main() {
    // Bone skinning
    mat4 skinMatrix = mat4(0.0);
    for (int i = 0; i < 4; i++) {
        if (aBoneIDs[i] >= 0) {
            skinMatrix += aBoneWeights[i] * boneTransforms[aBoneIDs[i]];
        }
    }
    // Fallback to identity if no bones
    if (skinMatrix == mat4(0.0)) skinMatrix = mat4(1.0);

    vec4 skinnedPos = skinMatrix * vec4(aPos, 1.0);
    vec4 worldPos = model * skinnedPos;
    vs_out.FragPos = worldPos.xyz;
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model * skinMatrix)));
    vs_out.Normal = normalize(normalMatrix * aNormal);

    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = vs_out.Normal;
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);
    vs_out.TBN = mat3(T, B, N);

    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPos;

    gl_Position = projection * view * worldPos;
}
