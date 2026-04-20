#version 460 core
// Minimal prepass vertex shader — writes depth + world-space normal.
// Doesn't compute TBN or FragPosLightSpace; those are main-pass
// concerns. Uses the same attribute layout as pbr_vertex.glsl so the
// same VBO binds without reconfiguration.
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 WorldNormal;
out vec2 TexCoords;

void main() {
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    WorldNormal = normalize(normalMatrix * aNormal);
    TexCoords   = aTexCoords;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
