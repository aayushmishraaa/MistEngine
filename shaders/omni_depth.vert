#version 460 core
// Omni shadow pass — single face per draw. Renderer iterates 6
// faces per light, rebinds viewProj for each.
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 viewProj;
out vec3 worldPos;
void main() {
    vec4 wp = model * vec4(aPos, 1.0);
    worldPos = wp.xyz;
    gl_Position = viewProj * wp;
}
