#version 410 core

in vec2 uv;
out vec4 FragColor;

uniform sampler2D accumTex;
uniform sampler2D revealTex;

const float EPSILON = 0.00001;

void main() {
    ivec2 coords = ivec2(gl_FragCoord.xy);
    float revealage = texelFetch(revealTex, coords, 0).r;

    if (abs(revealage - 1.0) < EPSILON)
        discard;

    vec4 accum = texelFetch(accumTex, coords, 0);

    if (isinf(max(max(abs(accum.r), abs(accum.g)), abs(accum.b))))
        accum.rgb = vec3(accum.a);

    vec3 avgColor = accum.rgb / max(accum.a, EPSILON);

    FragColor = vec4(avgColor, 1.0 - revealage);
}