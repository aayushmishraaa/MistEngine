#version 460 core
out float FragColor;
in vec2 TexCoords;

uniform sampler2D depthTexture;
uniform sampler2D noiseTexture;
// std140 pads vec3 to vec4 alignment, so we upload vec4 on the C++ side and
// unpack .xyz here. Binding 6 matches SSAORenderer::m_SamplesUBO.
layout(std140, binding = 6) uniform SSAOSamples {
    vec4 samples[64];
};
uniform mat4 projection;
uniform mat4 view;
uniform float radius;
uniform float bias;
uniform vec2 screenSize;

// Reconstruct view-space position from depth
vec3 getViewPos(vec2 uv) {
    float depth = texture(depthTexture, uv).r;
    // Convert to NDC
    vec4 ndc = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 viewPos = inverse(projection) * ndc;
    viewPos /= viewPos.w;
    return viewPos.xyz;
}

void main() {
    vec3 fragPos = getViewPos(TexCoords);

    // Reconstruct normal from depth derivatives
    vec3 dFdxPos = dFdx(fragPos);
    vec3 dFdyPos = dFdy(fragPos);
    vec3 normal = normalize(cross(dFdxPos, dFdyPos));

    // Noise for random rotation
    vec2 noiseScale = screenSize / 4.0;
    vec3 randomVec = texture(noiseTexture, TexCoords * noiseScale).xyz;

    // TBN from noise
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    int kernelSize = 64;

    for (int i = 0; i < kernelSize; ++i) {
        vec3 samplePos = TBN * samples[i].xyz;
        samplePos = fragPos + samplePos * radius;

        // Project sample to screen space
        vec4 offset = projection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5;

        float sampleDepth = getViewPos(offset.xy).z;

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - sampleDepth));
        occlusion += (sampleDepth >= samplePos.z + bias ? 1.0 : 0.0) * rangeCheck;
    }

    occlusion = 1.0 - (occlusion / float(kernelSize));
    FragColor = occlusion;
}
