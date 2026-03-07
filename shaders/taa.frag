#version 460 core

out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D currentFrame;
uniform sampler2D historyFrame;
uniform sampler2D velocityBuffer;
uniform vec2 inverseScreenSize;

// Catmull-Rom filtering for history sampling (reduces blur)
vec4 sampleCatmullRom(sampler2D tex, vec2 uv) {
    vec2 texSize = vec2(textureSize(tex, 0));
    vec2 samplePos = uv * texSize;
    vec2 texPos1 = floor(samplePos - 0.5) + 0.5;
    vec2 f = samplePos - texPos1;

    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);

    vec2 w12 = w1 + w2;
    vec2 offset12 = w2 / max(w12, 1e-6);

    vec2 tc0 = (texPos1 - 1.0) / texSize;
    vec2 tc3 = (texPos1 + 2.0) / texSize;
    vec2 tc12 = (texPos1 + offset12) / texSize;

    vec4 result = vec4(0.0);
    result += texture(tex, vec2(tc12.x, tc0.y)) * (w12.x * w0.y);
    result += texture(tex, vec2(tc0.x, tc12.y)) * (w0.x * w12.y);
    result += texture(tex, vec2(tc12.x, tc12.y)) * (w12.x * w12.y);
    result += texture(tex, vec2(tc3.x, tc12.y)) * (w3.x * w12.y);
    result += texture(tex, vec2(tc12.x, tc3.y)) * (w12.x * w3.y);

    return max(result, vec4(0.0));
}

// RGB -> YCoCg color space for better neighborhood clamping
vec3 RGBToYCoCg(vec3 rgb) {
    return vec3(
        0.25 * rgb.r + 0.5 * rgb.g + 0.25 * rgb.b,
        0.5 * rgb.r - 0.5 * rgb.b,
        -0.25 * rgb.r + 0.5 * rgb.g - 0.25 * rgb.b
    );
}

vec3 YCoCgToRGB(vec3 ycocg) {
    return vec3(
        ycocg.x + ycocg.y - ycocg.z,
        ycocg.x + ycocg.z,
        ycocg.x - ycocg.y - ycocg.z
    );
}

void main() {
    // Sample velocity at current pixel
    vec2 velocity = texture(velocityBuffer, TexCoords).rg;

    // Reprojected UV (where this pixel was last frame)
    vec2 historyUV = TexCoords - velocity;

    // Current frame color
    vec3 currentColor = texture(currentFrame, TexCoords).rgb;

    // Neighborhood clamping (3x3 min/max in YCoCg space)
    vec3 neighborMin = vec3(1e10);
    vec3 neighborMax = vec3(-1e10);

    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            vec2 offset = vec2(float(x), float(y)) * inverseScreenSize;
            vec3 neighbor = RGBToYCoCg(texture(currentFrame, TexCoords + offset).rgb);
            neighborMin = min(neighborMin, neighbor);
            neighborMax = max(neighborMax, neighbor);
        }
    }

    // Sample history with Catmull-Rom for sharpness
    vec3 historyColor;
    if (historyUV.x >= 0.0 && historyUV.x <= 1.0 && historyUV.y >= 0.0 && historyUV.y <= 1.0) {
        historyColor = sampleCatmullRom(historyFrame, historyUV).rgb;
    } else {
        // Off-screen: use current frame
        historyColor = currentColor;
    }

    // Clamp history to neighborhood in YCoCg
    vec3 historyYCoCg = RGBToYCoCg(historyColor);
    historyYCoCg = clamp(historyYCoCg, neighborMin, neighborMax);
    historyColor = YCoCgToRGB(historyYCoCg);

    // Velocity-based rejection: higher velocity = more current frame weight
    float velocityLength = length(velocity * vec2(textureSize(currentFrame, 0)));
    float velocityWeight = clamp(velocityLength / 2.0, 0.0, 1.0);

    // Blend factor: 0.05 = heavy temporal accumulation, higher = more responsive
    float blendFactor = mix(0.05, 0.3, velocityWeight);

    // Luminance-based weighting to reduce ghosting on bright objects
    float currentLuma = dot(currentColor, vec3(0.2126, 0.7152, 0.0722));
    float historyLuma = dot(historyColor, vec3(0.2126, 0.7152, 0.0722));
    float lumaDiff = abs(currentLuma - historyLuma) / max(currentLuma, max(historyLuma, 0.01));
    blendFactor = mix(blendFactor, 0.5, lumaDiff * 0.5);

    vec3 result = mix(historyColor, currentColor, blendFactor);
    FragColor = vec4(result, 1.0);
}
