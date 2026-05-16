#version 410 core

layout (location = 0) out vec3 outPosition;
layout (location = 1) out vec3 outNormal;

in vec3 vPosition;
in vec3 vNormal;

void main() {
    outPosition = vPosition;
    outNormal   = normalize(vNormal);
}
