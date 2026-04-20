#version 460 core
// FXAA — now operating in HDR (pre-tonemap) so specular highlights
// get anti-aliased before the tonemap operator clips them to white.
// The edge detector uses log-space luma to keep the gradient
// response proportional across the full HDR dynamic range; raw
// luma over-weights bright pixels and underweights dim ones.
//
// Reinhard-normalization before log2 avoids log(0) and caps the
// response curve for extreme HDR pixels (>1000 nits).
out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D screenTexture;
uniform vec2      inverseScreenSize;

#define FXAA_SPAN_MAX     8.0
#define FXAA_REDUCE_MUL   (1.0 / 8.0)
#define FXAA_REDUCE_MIN   (1.0 / 128.0)

// HDR-aware luma. Rec601 weights on Reinhard-normalized color, then
// log2 so large HDR values don't dominate the gradient. Tuned so
// LDR-like scenes (max ~1.0) behave close to the old rec601 luma.
float luma(vec3 c) {
    const vec3 w = vec3(0.299, 0.587, 0.114);
    float y = dot(max(c, vec3(0.0)), w);
    return log2(y / (1.0 + y) + 1e-4);
}

void main() {
    vec2 off = inverseScreenSize;

    float lumaTL = luma(texture(screenTexture, TexCoords + vec2(-1.0, -1.0) * off).rgb);
    float lumaTR = luma(texture(screenTexture, TexCoords + vec2( 1.0, -1.0) * off).rgb);
    float lumaBL = luma(texture(screenTexture, TexCoords + vec2(-1.0,  1.0) * off).rgb);
    float lumaBR = luma(texture(screenTexture, TexCoords + vec2( 1.0,  1.0) * off).rgb);
    float lumaM  = luma(texture(screenTexture, TexCoords).rgb);

    vec2 dir;
    dir.x = -((lumaTL + lumaTR) - (lumaBL + lumaBR));
    dir.y =  ((lumaTL + lumaBL) - (lumaTR + lumaBR));

    float dirReduce = max((lumaTL + lumaTR + lumaBL + lumaBR) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
    float rcpDirMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = min(vec2(FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX), dir * rcpDirMin)) * off;

    vec3 rgbA = 0.5 * (
        texture(screenTexture, TexCoords + dir * (1.0/3.0 - 0.5)).rgb +
        texture(screenTexture, TexCoords + dir * (2.0/3.0 - 0.5)).rgb);

    vec3 rgbB = rgbA * 0.5 + 0.25 * (
        texture(screenTexture, TexCoords + dir * -0.5).rgb +
        texture(screenTexture, TexCoords + dir *  0.5).rgb);

    float lumaB   = luma(rgbB);
    float lumaMin = min(lumaM, min(min(lumaTL, lumaTR), min(lumaBL, lumaBR)));
    float lumaMax = max(lumaM, max(max(lumaTL, lumaTR), max(lumaBL, lumaBR)));

    FragColor = ((lumaB < lumaMin) || (lumaB > lumaMax))
        ? vec4(rgbA, 1.0)
        : vec4(rgbB, 1.0);
}
