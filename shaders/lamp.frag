#version 330 core
out vec4 FragColor;

uniform vec3 lampColor; // Color of the lamp post

void main() {
    FragColor = vec4(lampColor, 1.0); // Solid color for the lamp post
}