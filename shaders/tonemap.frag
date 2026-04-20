#version 460 core
// Tonemap + gamma. Switches between ACES / Reinhard / AgX based on
// the uTonemapOp uniform:
//   0 = ACES filmic (prior default)
//   1 = Reinhard (cheapest, least saturated)
//   2 = AgX — Troy Sobotka's operator, Filament polynomial fit.
//       Godot switched to AgX as default in 4.3 for better hue
//       preservation and bloom rolloff. We follow suit; UIManager
//       defaults `tonemapOperator` to 2.
out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform float     exposure;
uniform int       uTonemapOp;

vec3 ACESFilm(vec3 x) {
    float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 Reinhard(vec3 x) {
    return x / (1.0 + x);
}

// AgX — polynomial fit. Reference: Filament's implementation of
// Troy Sobotka's sigmoid + input/output matrices. Produces more
// saturation and less hue-shift on bright warms than ACES.
vec3 AgX(vec3 c) {
    // Input transform (ACEScg-ish to AgX log) — Filament's
    // approximation uses a single matrix multiply.
    const mat3 kInput = mat3(
        0.842479062253094,  0.0423282422610123, 0.0423756549057051,
        0.0784335999999992, 0.878468636469772,  0.0784336,
        0.0792237451477643, 0.0791661274605434, 0.879142973793104);

    vec3 v = max(c, vec3(1e-6));
    v = log2(v);
    v = (v + 12.47393) / (12.47393 + 4.026069);   // AgX log range
    v = clamp(v, 0.0, 1.0);

    // Sobotka sigmoid (6th-order polynomial).
    vec3 x  = v;
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    v = + 15.5     * x4 * x2
        - 40.14    * x4 * x
        + 31.96    * x4
        -  6.868   * x2 * x
        +  0.4298  * x2
        +  0.1191  * x
        -  0.00232;

    // Output matrix (AgX to linear sRGB).
    const mat3 kOutput = mat3(
         1.19687900512017,   -0.0528968517574562, -0.0529716355144438,
        -0.0980208811401368,  1.15190312990417,   -0.0980434501171241,
        -0.0990297440797205, -0.0989611768448433,  1.15107367264116);
    return kOutput * v;
}

void main() {
    vec3 hdr = texture(hdrBuffer, TexCoords).rgb * exposure;
    vec3 mapped;
    if (uTonemapOp == 0)      mapped = ACESFilm(hdr);
    else if (uTonemapOp == 1) mapped = Reinhard(hdr);
    else                      mapped = AgX(hdr);

    // AgX and ACES output is already display-referred; Reinhard is
    // strictly linear. sRGB gamma comes last either way.
    mapped = pow(max(mapped, vec3(0.0)), vec3(1.0 / 2.2));
    FragColor = vec4(mapped, 1.0);
}
