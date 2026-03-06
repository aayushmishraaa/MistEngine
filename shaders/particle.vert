#version 460 core

struct Particle {
    vec4 position;  // xyz = pos, w = life
    vec4 velocity;  // xyz = vel, w = size
    vec4 color;     // rgba
};

layout(std430, binding = 0) buffer ParticleBuffer {
    Particle particles[];
};

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

out VS_OUT {
    vec4 color;
    vec2 texCoord;
    float life;
} vs_out;

void main() {
    Particle p = particles[gl_InstanceID];

    // Billboard quad corners from gl_VertexID (0-3 for triangle strip)
    vec2 offsets[4] = vec2[](
        vec2(-0.5, -0.5),
        vec2( 0.5, -0.5),
        vec2(-0.5,  0.5),
        vec2( 0.5,  0.5)
    );

    vec2 offset = offsets[gl_VertexID];
    float size = p.velocity.w;

    vec3 worldPos = p.position.xyz
        + cameraRight * offset.x * size
        + cameraUp * offset.y * size;

    vs_out.color = p.color;
    vs_out.texCoord = offset + 0.5;
    vs_out.life = p.position.w;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
