#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D colorTexture;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 invProjection;
uniform mat4 invView;

uniform float radius;       // World-space GI sample radius (default 3.0)
uniform float intensity;    // GI intensity multiplier (default 1.0)
uniform vec2 noiseScale;    // Screen dimensions / noise texture size

const int NUM_SAMPLES = 16;
const float PI = 3.14159265359;

// Hash function for pseudo-random sampling
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash2(vec2 p) {
    return vec2(hash(p), hash(p + vec2(127.1, 311.7)));
}

// Reconstruct view-space position from depth
vec3 viewPosFromDepth(vec2 uv, float depth) {
    vec4 clipPos = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = invProjection * clipPos;
    return viewPos.xyz / viewPos.w;
}

// Reconstruct view-space normal from depth buffer (if no explicit normal buffer)
vec3 reconstructNormal(vec2 uv) {
    vec2 texelSize = 1.0 / vec2(textureSize(depthTexture, 0));

    float dc = texture(depthTexture, uv).r;
    float dl = texture(depthTexture, uv - vec2(texelSize.x, 0.0)).r;
    float dr = texture(depthTexture, uv + vec2(texelSize.x, 0.0)).r;
    float db = texture(depthTexture, uv - vec2(0.0, texelSize.y)).r;
    float dt = texture(depthTexture, uv + vec2(0.0, texelSize.y)).r;

    vec3 pc = viewPosFromDepth(uv, dc);
    vec3 pl = viewPosFromDepth(uv - vec2(texelSize.x, 0.0), dl);
    vec3 pr = viewPosFromDepth(uv + vec2(texelSize.x, 0.0), dr);
    vec3 pb = viewPosFromDepth(uv - vec2(0.0, texelSize.y), db);
    vec3 pt = viewPosFromDepth(uv + vec2(0.0, texelSize.y), dt);

    // Use the closer neighbor for more robust normals at edges
    vec3 dx = (abs(dl - dc) < abs(dr - dc)) ? (pc - pl) : (pr - pc);
    vec3 dy = (abs(db - dc) < abs(dt - dc)) ? (pc - pb) : (pt - pc);

    return normalize(cross(dx, dy));
}

// Cosine-weighted hemisphere sample
vec3 sampleHemisphere(vec2 xi, vec3 normal) {
    float phi = 2.0 * PI * xi.x;
    float cosTheta = sqrt(1.0 - xi.y);
    float sinTheta = sqrt(xi.y);

    vec3 tangent = normalize(cross(normal, abs(normal.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0)));
    vec3 bitangent = cross(normal, tangent);

    return normalize(tangent * cos(phi) * sinTheta + bitangent * sin(phi) * sinTheta + normal * cosTheta);
}

void main() {
    float depth = texture(depthTexture, TexCoords).r;
    if (depth >= 1.0) {
        FragColor = vec4(0.0);
        return;
    }

    vec3 viewPos = viewPosFromDepth(TexCoords, depth);
    vec3 normal = reconstructNormal(TexCoords);

    vec3 indirectLight = vec3(0.0);
    float totalWeight = 0.0;

    // Screen-space trace for indirect illumination
    for (int i = 0; i < NUM_SAMPLES; i++) {
        // Generate random direction in hemisphere around normal
        vec2 xi = hash2(TexCoords * noiseScale + vec2(float(i) * 0.13, float(i) * 0.37));
        vec3 sampleDir = sampleHemisphere(xi, normal);

        // March along the direction in view space
        vec3 samplePos = viewPos + sampleDir * radius * (0.2 + 0.8 * hash(TexCoords + float(i)));

        // Project sample point to screen
        vec4 sampleClip = projection * vec4(samplePos, 1.0);
        vec2 sampleUV = (sampleClip.xy / sampleClip.w) * 0.5 + 0.5;

        // Check if sample is on screen
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;

        // Sample the depth at that screen position
        float sampleDepth = texture(depthTexture, sampleUV).r;
        vec3 sampleViewPos = viewPosFromDepth(sampleUV, sampleDepth);

        // Check if the sample hits geometry (depth test)
        float depthDiff = viewPos.z - sampleViewPos.z;
        if (depthDiff > 0.05 && depthDiff < radius) {
            // Sample the color at the hit point (indirect illumination source)
            vec3 hitColor = texture(colorTexture, sampleUV).rgb;

            // Distance-based attenuation
            float dist = length(sampleViewPos - viewPos);
            float attenuation = 1.0 / (1.0 + dist * dist);

            // Cosine weighting (already built into hemisphere sampling)
            float nDotL = max(dot(normal, sampleDir), 0.0);

            indirectLight += hitColor * attenuation * nDotL;
            totalWeight += 1.0;
        }
    }

    if (totalWeight > 0.0) {
        indirectLight /= totalWeight;
    }

    FragColor = vec4(indirectLight * intensity, 1.0);
}
