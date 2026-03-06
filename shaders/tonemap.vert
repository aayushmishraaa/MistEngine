#version 460 core

out vec2 TexCoords;

void main() {
    // Fullscreen triangle trick (no VBO needed)
    TexCoords = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(TexCoords * 2.0 - 1.0, 0.0, 1.0);
}
