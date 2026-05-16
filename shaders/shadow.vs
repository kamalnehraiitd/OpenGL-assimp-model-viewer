#version 410 core

layout (location = 0) in vec3 aPos;

uniform mat4 viewToLightClip;
uniform mat4 viewModel;

void main() {

    gl_Position = viewToLightClip * viewModel * vec4(aPos, 1.0);
}
