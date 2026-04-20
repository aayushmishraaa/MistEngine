#version 460 core
// DOF composite — upsample half-res bokeh back to full-res and lerp
// against the original sharp HDR based on CoC magnitude. Pixels at
// |CoC| < 0.5 stay sharp; larger CoC picks the bokeh sample.
out vec4 FragColor;
in  vec2 TexCoords;

uniform sampler2D sharpHDR;     // full-res, pre-DOF
uniform sampler2D bokehHDR;     // half-res, post-bokeh
uniform sampler2D cocTex;       // full-res CoC

void main() {
    vec3 sharp = texture(sharpHDR, TexCoords).rgb;
    vec3 blur  = texture(bokehHDR, TexCoords).rgb;   // linear filter upsample
    float coc  = abs(texture(cocTex, TexCoords).r);

    // Smooth transition band around |CoC| ~ 1 px — avoids a sharp
    // silhouette where in-focus and bokeh'd pixels meet.
    float t = smoothstep(0.5, 2.0, coc);
    FragColor = vec4(mix(sharp, blur, t), 1.0);
}
