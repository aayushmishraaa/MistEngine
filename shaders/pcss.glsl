// PCSS (Percentage-Closer Soft Shadows) functions
// Include this in your PBR fragment shader

const int PCSS_BLOCKER_SAMPLES = 16;
const int PCSS_PCF_SAMPLES = 32;
const float PCSS_LIGHT_SIZE = 0.04;  // Light source size (larger = softer)

// Poisson disk samples for blocker search and PCF
const vec2 poissonDisk16[16] = vec2[](
    vec2(-0.94201624, -0.39906216),  vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),   vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),   vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),   vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),  vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),   vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),    vec2(0.14383161, -0.14100790)
);

const vec2 poissonDisk32[32] = vec2[](
    vec2(-0.975402, -0.0711386), vec2(-0.920347, -0.41142),
    vec2(-0.883908, 0.217872),   vec2(-0.884518, 0.568041),
    vec2(-0.811945, 0.90521),    vec2(-0.792474, -0.779962),
    vec2(-0.614856, 0.386578),   vec2(-0.580859, -0.208777),
    vec2(-0.53795, 0.716666),    vec2(-0.515427, 0.0899991),
    vec2(-0.454634, -0.707938),  vec2(-0.420942, -0.429198),
    vec2(-0.300778, -0.162162),  vec2(-0.272889, 0.505945),
    vec2(-0.169756, -0.586),     vec2(-0.0620197, 0.27719),
    vec2(-0.0244465, 0.940627),  vec2(0.0343762, -0.201581),
    vec2(0.0504932, -0.854536),  vec2(0.105539, 0.590166),
    vec2(0.207631, -0.0710704),  vec2(0.208819, 0.273364),
    vec2(0.287755, -0.629961),   vec2(0.348902, 0.689759),
    vec2(0.405676, -0.353282),   vec2(0.456454, 0.0683878),
    vec2(0.553899, -0.816737),   vec2(0.594618, 0.432504),
    vec2(0.673298, -0.212688),   vec2(0.730373, 0.0740507),
    vec2(0.799201, 0.557664),    vec2(0.869508, -0.516457)
);

// Pseudorandom rotation per-pixel for temporal stability
float interleavedGradientNoise(vec2 screenPos) {
    vec3 magic = vec3(0.06711056, 0.00583715, 52.9829189);
    return fract(magic.z * fract(dot(screenPos, magic.xy)));
}

mat2 randomRotation(vec2 screenPos) {
    float angle = interleavedGradientNoise(screenPos) * 6.283185;
    float s = sin(angle);
    float c = cos(angle);
    return mat2(c, -s, s, c);
}

// Step 1: Find average blocker depth
float findBlockerDepth(sampler2DArray shadowMap, int cascade, vec3 projCoords, float currentDepth, float searchRadius) {
    float blockerSum = 0.0;
    int numBlockers = 0;

    mat2 rotation = randomRotation(gl_FragCoord.xy);

    for (int i = 0; i < PCSS_BLOCKER_SAMPLES; i++) {
        vec2 offset = rotation * poissonDisk16[i] * searchRadius;
        float sampleDepth = texture(shadowMap, vec3(projCoords.xy + offset, float(cascade))).r;

        if (sampleDepth < currentDepth - 0.002) {
            blockerSum += sampleDepth;
            numBlockers++;
        }
    }

    if (numBlockers == 0) return -1.0; // No blockers found
    return blockerSum / float(numBlockers);
}

// Step 2: Estimate penumbra size from blocker distance
float estimatePenumbra(float receiverDepth, float blockerDepth) {
    return PCSS_LIGHT_SIZE * (receiverDepth - blockerDepth) / blockerDepth;
}

// Step 3: PCF with variable kernel
float pcfFilter(sampler2DArray shadowMap, int cascade, vec3 projCoords, float currentDepth, float filterRadius) {
    float shadow = 0.0;
    mat2 rotation = randomRotation(gl_FragCoord.xy);

    for (int i = 0; i < PCSS_PCF_SAMPLES; i++) {
        vec2 offset = rotation * poissonDisk32[i] * filterRadius;
        float sampleDepth = texture(shadowMap, vec3(projCoords.xy + offset, float(cascade))).r;
        shadow += (currentDepth - 0.002 > sampleDepth) ? 1.0 : 0.0;
    }

    return shadow / float(PCSS_PCF_SAMPLES);
}

// Full PCSS calculation
float PCSS(sampler2DArray shadowMap, int cascade, vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // Step 1: Blocker search
    float searchRadius = PCSS_LIGHT_SIZE * currentDepth / 2.0;
    float blockerDepth = findBlockerDepth(shadowMap, cascade, projCoords, currentDepth, searchRadius);

    if (blockerDepth < 0.0) return 0.0; // No shadow

    // Step 2: Penumbra estimation
    float penumbraWidth = estimatePenumbra(currentDepth, blockerDepth);

    // Step 3: Filtering
    float filterRadius = penumbraWidth * 0.01; // Scale to shadow map texel space
    filterRadius = clamp(filterRadius, 0.001, 0.02);

    return pcfFilter(shadowMap, cascade, projCoords, currentDepth, filterRadius);
}
