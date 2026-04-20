#version 460 core
// Circle-of-confusion (CoC) pass. Reads screen-space depth, writes
// a single R16F channel carrying signed blur radius per pixel.
//
// Positive CoC = pixel is behind the focus plane (far blur).
// Negative CoC = pixel is in front of the focus plane (near blur).
// The composite pass uses the magnitude; sign is preserved for
// future use (split near/far passes for better DOF quality).
//
// All math in view-space linear depth. Depth is reconstructed from
// the depth buffer via inverse projection.
out float FragColor;
in  vec2  TexCoords;

uniform sampler2D depthTex;
uniform mat4      uInvProj;
uniform float     uFocusDistance;  // world-space distance (view-space -Z)
uniform float     uAperture;       // 0..1, scales CoC
uniform float     uMaxRadius;      // clamp in pixels

float viewZFromDepth(vec2 uv, float depth) {
    // Rebuild a clip-space point, project to view-space, take Z.
    vec4 clip = vec4(uv * 2.0 - 1.0, depth * 2.0 - 1.0, 1.0);
    vec4 v    = uInvProj * clip;
    return -v.z / v.w;    // positive distance along camera forward
}

void main() {
    float depth = texture(depthTex, TexCoords).r;
    if (depth >= 1.0) { FragColor = 0.0; return; }

    float viewZ = viewZFromDepth(TexCoords, depth);
    // Physically-motivated CoC: aperture * |focus - z| / z. Scaled
    // by uMaxRadius so user can tune peak blur without redoing the
    // focal math.
    float coc = uAperture * (viewZ - uFocusDistance) / max(viewZ, 1e-3);
    FragColor = clamp(coc * uMaxRadius, -uMaxRadius, uMaxRadius);
}
