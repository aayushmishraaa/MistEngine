#version 460 core
// Bokeh DOF — half-res disk sampler. For each half-res pixel, sample
// the scene HDR at N offsets along a Vogel disk, kernel radius =
// max CoC in the 2x2 block we downsampled from.
//
// Vogel disk (golden-angle spiral) gives even coverage with few
// samples and no banding. 16 taps is the standard compromise between
// cost and quality; Godot uses 32 in high quality mode.
out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D sceneHDR;   // full-res HDR scene
uniform sampler2D cocTex;     // full-res CoC (R16F, signed)
uniform vec2      uHalfResInvSize;   // 1 / halfResW, 1 / halfResH

const int   N_TAPS = 16;
const float GOLDEN_ANGLE = 2.39996322972865332;  // pi * (3 - sqrt(5))

vec2 vogel(int i, int n, float phi) {
    float r = sqrt((float(i) + 0.5) / float(n));
    float theta = float(i) * GOLDEN_ANGLE + phi;
    return vec2(cos(theta), sin(theta)) * r;
}

void main() {
    // At half-res, one texel covers a 2x2 region of full-res. Use
    // the max CoC magnitude in that block so depth-edge pixels don't
    // lose their bokeh from a neighbor that was in-focus.
    float c0 = abs(texture(cocTex, TexCoords + vec2(-0.5, -0.5) * uHalfResInvSize * 0.5).r);
    float c1 = abs(texture(cocTex, TexCoords + vec2( 0.5, -0.5) * uHalfResInvSize * 0.5).r);
    float c2 = abs(texture(cocTex, TexCoords + vec2(-0.5,  0.5) * uHalfResInvSize * 0.5).r);
    float c3 = abs(texture(cocTex, TexCoords + vec2( 0.5,  0.5) * uHalfResInvSize * 0.5).r);
    float radius = max(max(c0, c1), max(c2, c3));

    if (radius < 0.5) {
        // In-focus — sample current pixel only. Cheaper than 16 taps
        // for the bulk of the image.
        FragColor = texture(sceneHDR, TexCoords);
        return;
    }

    // Randomize the disk's starting angle per pixel so the pattern
    // doesn't tile visibly across the screen. Cheap hash on gl_FragCoord.
    float phi = fract(sin(dot(gl_FragCoord.xy, vec2(12.9898, 78.233))) * 43758.5453) * 6.28318;

    // Full-res size — derive from the half-res inverse so we don't
    // need another uniform.
    vec2 fullInvSize = uHalfResInvSize * 0.5;

    vec3 accum = vec3(0.0);
    for (int i = 0; i < N_TAPS; ++i) {
        vec2 off = vogel(i, N_TAPS, phi) * radius * fullInvSize;
        accum += texture(sceneHDR, TexCoords + off).rgb;
    }
    FragColor = vec4(accum / float(N_TAPS), 1.0);
}
