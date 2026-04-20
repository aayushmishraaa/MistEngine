#version 460 core
// Prepass fragment shader — writes packed normal + roughness into
// a single RGBA16F MRT color attachment. Depth goes to the FBO's
// depth attachment implicitly.
//
// Normal encoding: octahedral (2 channels). RGB normals would waste
// a channel on sign; octahedral gives near-identical quality in 2
// channels and packs into .rg cleanly.
//
// Spare channel (.a) reserved — could carry material-ID, shading
// model flags, or velocity fallback data in later cycles.

in vec3 WorldNormal;
in vec2 TexCoords;

uniform sampler2D roughnessMap;
uniform float     roughnessValue;
uniform bool      hasRoughnessMap;

out vec4 FragColor;

vec2 octEncode(vec3 n) {
    // From "Survey of Efficient Representations for Independent Unit
    // Vectors" (Cigolle et al. 2014). `abs().sum()` L1-normalizes n,
    // then the reflection for the back hemisphere keeps the mapping
    // bijective.
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    vec2 enc = (n.z >= 0.0)
        ? n.xy
        : (1.0 - abs(n.yx)) * vec2(
            n.x >= 0.0 ? 1.0 : -1.0,
            n.y >= 0.0 ? 1.0 : -1.0);
    return enc * 0.5 + 0.5;
}

void main() {
    vec3 n = normalize(WorldNormal);
    float r = hasRoughnessMap ? texture(roughnessMap, TexCoords).r
                              : roughnessValue;
    FragColor = vec4(octEncode(n), r, 0.0);
}
