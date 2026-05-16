#version 410 core

in vec2 uv;
out vec4 FragColor;

uniform sampler2D peelLayer;

void main() {
    FragColor = texture(peelLayer, uv);
}
