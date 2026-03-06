#version 460 core
out vec4 FragColor;
in vec3 WorldPos;

uniform vec3 sunDirection;
uniform float rayleighStrength;
uniform float mieStrength;
uniform float turbidity;

const float PI = 3.14159265359;
const vec3 betaR = vec3(5.5e-6, 13.0e-6, 22.4e-6); // Rayleigh scattering
const vec3 betaM = vec3(21e-6);                       // Mie scattering

float rayleighPhase(float cosTheta) {
    return (3.0 / (16.0 * PI)) * (1.0 + cosTheta * cosTheta);
}

float miePhase(float cosTheta, float g) {
    float gg = g * g;
    return (3.0 / (8.0 * PI)) * ((1.0 - gg) * (1.0 + cosTheta * cosTheta)) /
           (pow(1.0 + gg - 2.0 * g * cosTheta, 1.5) * (2.0 + gg));
}

void main() {
    vec3 direction = normalize(WorldPos);
    float cosTheta = dot(direction, normalize(sunDirection));

    // Simple single-scattering approximation
    float zenithAngle = max(0.0, direction.y);
    float opticalDepth = 1.0 / (zenithAngle + 0.15 * pow(93.885 - degrees(acos(zenithAngle)), -1.253));

    vec3 rayleigh = betaR * rayleighPhase(cosTheta) * rayleighStrength * opticalDepth;
    vec3 mie = betaM * miePhase(cosTheta, 0.76) * mieStrength * turbidity * opticalDepth;

    vec3 extinction = exp(-(betaR * rayleighStrength + betaM * mieStrength * turbidity) * opticalDepth);
    vec3 inscatter = (rayleigh + mie) * (1.0 - extinction);

    // Sun disk
    float sunDisk = smoothstep(0.9995, 0.9999, cosTheta) * 50.0;
    vec3 sunColor = vec3(1.0, 0.9, 0.7) * sunDisk;

    vec3 color = inscatter * 20.0 + sunColor;

    // Ground darkening
    if (direction.y < 0.0) {
        color = mix(color, vec3(0.1, 0.1, 0.12), smoothstep(0.0, -0.3, direction.y));
    }

    FragColor = vec4(color, 1.0);
}
