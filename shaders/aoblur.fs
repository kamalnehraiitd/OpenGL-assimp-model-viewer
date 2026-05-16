#version 410 core

out float FragColor;
in vec2 uv;

uniform sampler2D aoInput;

void main() {
    vec2 step = 1.0 / vec2(textureSize(aoInput, 0));

    float sum = 0.0;
    for (int dx = -2; dx <= 2; ++dx) {
        for (int dy = -2; dy <= 2; ++dy) {
            sum += texture(aoInput, uv + vec2(float(dx), float(dy)) * step).r;
        }
    }

    FragColor = sum / 25.0;
}
