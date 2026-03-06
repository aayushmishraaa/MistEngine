#version 460 core

in VS_OUT {
    vec4 color;
    vec2 texCoord;
    float life;
} fs_in;

uniform sampler2D particleTexture;
uniform sampler2D depthTexture;
uniform bool useTexture;
uniform bool softParticles;
uniform float softDistance = 0.5;
uniform vec2 screenSize;
uniform float nearPlane;
uniform float farPlane;

out vec4 FragColor;

float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane) / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main() {
    vec4 color = fs_in.color;

    if (useTexture) {
        color *= texture(particleTexture, fs_in.texCoord);
    } else {
        // Soft circle falloff
        float dist = length(fs_in.texCoord - vec2(0.5));
        float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
        color.a *= alpha;
    }

    // Fade out near end of life
    color.a *= smoothstep(0.0, 0.1, fs_in.life);

    // Soft particle depth fade
    if (softParticles) {
        vec2 screenUV = gl_FragCoord.xy / screenSize;
        float sceneDepth = linearizeDepth(texture(depthTexture, screenUV).r);
        float particleDepth = linearizeDepth(gl_FragCoord.z);
        float depthDiff = sceneDepth - particleDepth;
        float softFade = smoothstep(0.0, softDistance, depthDiff);
        color.a *= softFade;
    }

    if (color.a < 0.01) discard;

    FragColor = color;
}
