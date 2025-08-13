#version 330 core
out vec4 FragColor;

in vec3 TexCoords;

void main()
{
    vec3 direction = normalize(TexCoords);
    
    // Create a Unity-like gradient from horizon to sky
    vec3 horizonColor = vec3(0.8, 0.9, 1.0);   // Light blue horizon
    vec3 zenithColor = vec3(0.2, 0.5, 1.0);    // Deeper blue sky
    vec3 groundColor = vec3(0.3, 0.3, 0.4);    // Dark ground
    
    float t = direction.y; // Use Y component for vertical gradient
    
    vec3 skyColor;
    if (t > 0.0) {
        // Above horizon - blend from horizon to zenith
        skyColor = mix(horizonColor, zenithColor, t);
    } else {
        // Below horizon - blend from horizon to ground
        skyColor = mix(horizonColor, groundColor, -t);
    }
    
    FragColor = vec4(skyColor, 1.0);
}