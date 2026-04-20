#version 460 core
// Omni shadow pass fragment. Outputs linearized distance from the
// light rather than clip-space depth — the PBR shader compares
// against raw world-space distance, which is simpler than
// reconstructing the non-linear projection depth.
in  vec3 worldPos;
uniform vec3  lightPos;
uniform float farPlane;

void main() {
    float dist = length(worldPos - lightPos);
    gl_FragDepth = dist / farPlane;   // normalize to [0,1]
}
