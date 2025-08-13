#version 330 core
out vec4 FragColor;

uniform vec3 orbColor;
uniform vec3 orbPosition;

void main()
{
    // Create a glowing effect
    vec3 color = orbColor * 2.0; // Brighten the color
    FragColor = vec4(color, 1.0); // Full opacity for better visibility
}