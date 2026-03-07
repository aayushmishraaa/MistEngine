#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D ssgiTexture;
uniform sampler2D depthTexture;
uniform vec2 direction; // (1,0) for horizontal, (0,1) for vertical

const int KERNEL_SIZE = 4;
const float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main() {
    vec2 texelSize = 1.0 / vec2(textureSize(ssgiTexture, 0));
    float centerDepth = texture(depthTexture, TexCoords).r;

    vec3 result = texture(ssgiTexture, TexCoords).rgb * weights[0];
    float totalWeight = weights[0];

    for (int i = 1; i <= KERNEL_SIZE; i++) {
        vec2 offset = direction * texelSize * float(i);

        // Bilateral weight: reject samples with very different depth
        float depthP = texture(depthTexture, TexCoords + offset).r;
        float depthN = texture(depthTexture, TexCoords - offset).r;

        float wp = exp(-abs(depthP - centerDepth) * 1000.0) * weights[i];
        float wn = exp(-abs(depthN - centerDepth) * 1000.0) * weights[i];

        result += texture(ssgiTexture, TexCoords + offset).rgb * wp;
        result += texture(ssgiTexture, TexCoords - offset).rgb * wn;
        totalWeight += wp + wn;
    }

    FragColor = vec4(result / totalWeight, 1.0);
}
