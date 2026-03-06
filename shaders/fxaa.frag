#version 460 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2 inverseScreenSize;

#define FXAA_SPAN_MAX 8.0
#define FXAA_REDUCE_MUL (1.0/8.0)
#define FXAA_REDUCE_MIN (1.0/128.0)

void main() {
    vec2 texCoordOffset = inverseScreenSize;

    vec3 luma = vec3(0.299, 0.587, 0.114);

    float lumaTL = dot(luma, texture(screenTexture, TexCoords + vec2(-1.0, -1.0) * texCoordOffset).rgb);
    float lumaTR = dot(luma, texture(screenTexture, TexCoords + vec2( 1.0, -1.0) * texCoordOffset).rgb);
    float lumaBL = dot(luma, texture(screenTexture, TexCoords + vec2(-1.0,  1.0) * texCoordOffset).rgb);
    float lumaBR = dot(luma, texture(screenTexture, TexCoords + vec2( 1.0,  1.0) * texCoordOffset).rgb);
    float lumaM  = dot(luma, texture(screenTexture, TexCoords).rgb);

    vec2 dir;
    dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
    dir.y =  ((lumaTL + lumaBL) - (lumaTR + lumaBR));

    float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);

    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * texCoordOffset;

    vec3 rgbA = 0.5 * (
        texture(screenTexture, TexCoords + dir * (1.0/3.0 - 0.5)).rgb +
        texture(screenTexture, TexCoords + dir * (2.0/3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, TexCoords + dir * -0.5).rgb +
        texture(screenTexture, TexCoords + dir *  0.5).rgb);

    float lumaB = dot(rgbB, luma);

    float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
    float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));

    if ((lumaB < lumaMin) || (lumaB > lumaMax)) {
        FragColor = vec4(rgbA, 1.0);
    } else {
        FragColor = vec4(rgbB, 1.0);
    }
}
