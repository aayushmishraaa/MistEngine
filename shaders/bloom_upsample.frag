#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D srcTexture;
uniform vec2 srcResolution;
uniform float bloomIntensity;

void main() {
    // 9-tap tent filter for smooth upsample
    vec2 texelSize = 1.0 / srcResolution;

    vec3 color = vec3(0.0);
    color += texture(srcTexture, TexCoords + vec2(-texelSize.x, texelSize.y)).rgb;
    color += texture(srcTexture, TexCoords + vec2( 0.0,         texelSize.y)).rgb * 2.0;
    color += texture(srcTexture, TexCoords + vec2( texelSize.x, texelSize.y)).rgb;

    color += texture(srcTexture, TexCoords + vec2(-texelSize.x, 0.0)).rgb * 2.0;
    color += texture(srcTexture, TexCoords).rgb * 4.0;
    color += texture(srcTexture, TexCoords + vec2( texelSize.x, 0.0)).rgb * 2.0;

    color += texture(srcTexture, TexCoords + vec2(-texelSize.x,-texelSize.y)).rgb;
    color += texture(srcTexture, TexCoords + vec2( 0.0,        -texelSize.y)).rgb * 2.0;
    color += texture(srcTexture, TexCoords + vec2( texelSize.x,-texelSize.y)).rgb;

    color /= 16.0;

    FragColor = vec4(color * bloomIntensity, 1.0);
}
