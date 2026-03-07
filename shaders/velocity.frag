#version 460 core

in vec4 clipPos;
in vec4 prevClipPos;

layout (location = 0) out vec2 Velocity;

void main() {
    // Convert clip space to NDC
    vec2 ndcPos = clipPos.xy / clipPos.w;
    vec2 prevNdcPos = prevClipPos.xy / prevClipPos.w;

    // Convert NDC to UV space [0,1]
    vec2 currentUV = ndcPos * 0.5 + 0.5;
    vec2 prevUV = prevNdcPos * 0.5 + 0.5;

    // Velocity = where the pixel moved from (current - previous)
    Velocity = currentUV - prevUV;
}
