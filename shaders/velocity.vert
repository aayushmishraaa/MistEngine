#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 prevModel;
uniform mat4 viewProjection;
uniform mat4 prevViewProjection;
uniform vec2 jitter;
uniform vec2 prevJitter;

out vec4 clipPos;
out vec4 prevClipPos;

void main() {
    vec4 worldPos = model * vec4(aPos, 1.0);
    vec4 prevWorldPos = prevModel * vec4(aPos, 1.0);

    clipPos = viewProjection * worldPos;
    prevClipPos = prevViewProjection * prevWorldPos;

    // Apply jitter for TAA subpixel offset
    gl_Position = clipPos;
    gl_Position.xy += jitter * gl_Position.w;
}
