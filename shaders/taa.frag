#version 460 core
// TAA resolve — rewritten following Salvi 2016 + Karis. Two upgrades
// over the prior YCoCg min-max clamp:
//
//   1) Closest-depth 3x3 velocity: the 9 neighbors' depths are scanned
//      and the velocity from the depth-closest neighbor is used. Fixes
//      silhouette ghosting where the central pixel is occluded but a
//      neighbor carries the right motion vector.
//
//   2) Variance clipping: the history is clipped to [mean - k*stddev,
//      mean + k*stddev] of the 3x3 current-frame neighborhood rather
//      than to its min/max AABB. The AABB snaps hard when one bright
//      pixel enters the kernel; variance clipping scales proportional
//      to local contrast. k = 1.25 is Godot's default.
//
// The clip happens in Reinhard-tonemapped space so one bright HDR
// sample can't dominate the mean; we untonemap after clipping.

out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D currentFrame;
uniform sampler2D historyFrame;
uniform sampler2D velocityBuffer;
uniform sampler2D depthBuffer;    // prepass depth — closest-depth picker
uniform vec2      inverseScreenSize;

const float VARIANCE_K = 1.25;

// Catmull-Rom — unchanged from the prior implementation. Sharpens the
// history sample vs. a plain bilinear, hiding the temporal blur.
vec4 sampleCatmullRom(sampler2D tex, vec2 uv) {
    vec2 texSize   = vec2(textureSize(tex, 0));
    vec2 samplePos = uv * texSize;
    vec2 texPos1   = floor(samplePos - 0.5) + 0.5;
    vec2 f         = samplePos - texPos1;
    vec2 w0 = f * (-0.5 + f * (1.0 - 0.5 * f));
    vec2 w1 = 1.0 + f * f * (-2.5 + 1.5 * f);
    vec2 w2 = f * (0.5 + f * (2.0 - 1.5 * f));
    vec2 w3 = f * f * (-0.5 + 0.5 * f);
    vec2 w12      = w1 + w2;
    vec2 offset12 = w2 / max(w12, vec2(1e-6));
    vec2 tc0  = (texPos1 - 1.0) / texSize;
    vec2 tc3  = (texPos1 + 2.0) / texSize;
    vec2 tc12 = (texPos1 + offset12) / texSize;
    vec4 r = vec4(0.0);
    r += texture(tex, vec2(tc12.x, tc0.y )) * (w12.x * w0.y );
    r += texture(tex, vec2(tc0.x , tc12.y)) * (w0.x  * w12.y);
    r += texture(tex, vec2(tc12.x, tc12.y)) * (w12.x * w12.y);
    r += texture(tex, vec2(tc3.x , tc12.y)) * (w3.x  * w12.y);
    r += texture(tex, vec2(tc12.x, tc3.y )) * (w12.x * w3.y );
    return max(r, vec4(0.0));
}

// Reinhard tonemap for clipping-space luminance. Cheap and reversible
// (within float precision). Nothing leaves the shader in tonemapped
// space — we invert after clipping.
vec3 reinhard(vec3 c)     { return c / (1.0 + c); }
vec3 invReinhard(vec3 c)  { return c / max(1.0 - c, vec3(1e-4)); }

// Closest-depth velocity: scan 3x3 neighborhood, return velocity at
// the pixel with smallest depth (nearest to camera). Central pixel
// is included. If depthBuffer is unbound (size ~ 1), fall back to
// central-pixel velocity.
vec2 closestDepthVelocity(vec2 uv) {
    ivec2 px = ivec2(uv * vec2(textureSize(currentFrame, 0)));
    ivec2 bestPx = px;
    float bestDepth = texelFetch(depthBuffer, px, 0).r;
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            ivec2 n = px + ivec2(dx, dy);
            float d = texelFetch(depthBuffer, n, 0).r;
            if (d < bestDepth) { bestDepth = d; bestPx = n; }
        }
    }
    return texelFetch(velocityBuffer, bestPx, 0).rg;
}

void main() {
    vec2 velocity   = closestDepthVelocity(TexCoords);
    vec2 historyUV  = TexCoords - velocity;

    vec3 currentColor = texture(currentFrame, TexCoords).rgb;

    // Variance stats over 3x3 current-frame neighborhood in Reinhard
    // space. AABB min/max (used by the old implementation) lived in
    // YCoCg; variance lives in Reinhard-tonemapped RGB — direct to
    // implement, and the clip is chroma-aware enough for our needs.
    vec3 sum     = vec3(0.0);
    vec3 sumSq   = vec3(0.0);
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            vec2 off = vec2(float(dx), float(dy)) * inverseScreenSize;
            vec3 c   = reinhard(texture(currentFrame, TexCoords + off).rgb);
            sum     += c;
            sumSq   += c * c;
        }
    }
    vec3 mean   = sum   / 9.0;
    vec3 meanSq = sumSq / 9.0;
    vec3 stddev = sqrt(max(meanSq - mean * mean, vec3(0.0)));
    vec3 lo = mean - VARIANCE_K * stddev;
    vec3 hi = mean + VARIANCE_K * stddev;

    // History sample — Catmull-Rom, then off-screen reject.
    vec3 historyColor;
    if (historyUV.x >= 0.0 && historyUV.x <= 1.0 &&
        historyUV.y >= 0.0 && historyUV.y <= 1.0) {
        historyColor = sampleCatmullRom(historyFrame, historyUV).rgb;
    } else {
        historyColor = currentColor;
    }

    vec3 historyReinhard = reinhard(historyColor);
    vec3 clippedHistory  = invReinhard(clamp(historyReinhard, lo, hi));

    // Velocity-adaptive blend — old logic preserved. Fast motion
    // leans on current frame; stationary pixels accumulate history.
    float velocityLength = length(velocity * vec2(textureSize(currentFrame, 0)));
    float velocityWeight = clamp(velocityLength / 2.0, 0.0, 1.0);
    float blendFactor    = mix(0.05, 0.3, velocityWeight);

    // Luma-diff rejection kept from prior resolve — cheap and
    // complements variance clipping on bright-object transitions.
    float currentLuma = dot(currentColor,   vec3(0.2126, 0.7152, 0.0722));
    float historyLuma = dot(clippedHistory, vec3(0.2126, 0.7152, 0.0722));
    float lumaDiff    = abs(currentLuma - historyLuma) /
                        max(currentLuma, max(historyLuma, 0.01));
    blendFactor = mix(blendFactor, 0.5, lumaDiff * 0.5);

    vec3 result = mix(clippedHistory, currentColor, blendFactor);
    FragColor   = vec4(result, 1.0);
}
