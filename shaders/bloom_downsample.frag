#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D srcTexture;
uniform vec2 srcResolution;
uniform int mipLevel;
uniform float threshold;

void main() {
    vec2 texelSize = 1.0 / srcResolution;

    // 13-tap box filter (Karis average for first mip)
    vec3 A = texture(srcTexture, TexCoords + texelSize * vec2(-2.0, 2.0)).rgb;
    vec3 B = texture(srcTexture, TexCoords + texelSize * vec2( 0.0, 2.0)).rgb;
    vec3 C = texture(srcTexture, TexCoords + texelSize * vec2( 2.0, 2.0)).rgb;
    vec3 D = texture(srcTexture, TexCoords + texelSize * vec2(-2.0, 0.0)).rgb;
    vec3 E = texture(srcTexture, TexCoords).rgb;
    vec3 F = texture(srcTexture, TexCoords + texelSize * vec2( 2.0, 0.0)).rgb;
    vec3 G = texture(srcTexture, TexCoords + texelSize * vec2(-2.0,-2.0)).rgb;
    vec3 H = texture(srcTexture, TexCoords + texelSize * vec2( 0.0,-2.0)).rgb;
    vec3 I = texture(srcTexture, TexCoords + texelSize * vec2( 2.0,-2.0)).rgb;
    vec3 J = texture(srcTexture, TexCoords + texelSize * vec2(-1.0, 1.0)).rgb;
    vec3 K = texture(srcTexture, TexCoords + texelSize * vec2( 1.0, 1.0)).rgb;
    vec3 L = texture(srcTexture, TexCoords + texelSize * vec2(-1.0,-1.0)).rgb;
    vec3 M = texture(srcTexture, TexCoords + texelSize * vec2( 1.0,-1.0)).rgb;

    vec3 color = E * 0.125;
    color += (A + C + G + I) * 0.03125;
    color += (B + D + F + H) * 0.0625;
    color += (J + K + L + M) * 0.125;

    // Apply brightness threshold on first downsample
    if (mipLevel == 0 && threshold > 0.0) {
        float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
        if (brightness < threshold) {
            color *= smoothstep(threshold * 0.8, threshold, brightness);
        }
    }

    FragColor = vec4(color, 1.0);
}
