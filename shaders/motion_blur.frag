#version 460 core
// Simple N-tap camera/object motion blur.
//
// Samples the HDR scene along the per-pixel velocity vector. Godot
// uses 6 taps in its Forward+ pipeline; we follow suit. The velocity
// is scaled by `uStrength` so the user can dial the effect without
// re-rendering motion vectors.
out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D sceneHDR;
uniform sampler2D velocityBuffer;
uniform float     uStrength;

const int N_TAPS = 6;

void main() {
    vec2 v = texture(velocityBuffer, TexCoords).rg * uStrength;

    // Short-circuit when motion is negligible — avoids N taps just
    // to return `base` again, and keeps static-camera performance
    // honest.
    if (dot(v, v) < 1e-8) {
        FragColor = texture(sceneHDR, TexCoords);
        return;
    }

    vec3 accum = texture(sceneHDR, TexCoords).rgb;
    for (int i = 1; i < N_TAPS; ++i) {
        // Offsets span -0.5..+0.5 of the velocity vector, centered
        // on the current pixel. Symmetric trail around the current
        // position looks more natural than a one-sided sample.
        float t = float(i) / float(N_TAPS - 1) - 0.5;
        vec2 uv = TexCoords + v * t;
        accum += texture(sceneHDR, uv).rgb;
    }
    FragColor = vec4(accum / float(N_TAPS), 1.0);
}
