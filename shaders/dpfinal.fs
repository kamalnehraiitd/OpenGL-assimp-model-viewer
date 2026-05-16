#version 410 core

in vec2 uv;
out vec4 FragColor;

uniform sampler2D accumTex;

void main() {
    vec4 accum = texture(accumTex, uv);
    if (accum.a > 0.999 && length(accum.rgb) < 0.001)
        discard;
    FragColor = vec4(accum.rgb, 1.0 - accum.a);
}
