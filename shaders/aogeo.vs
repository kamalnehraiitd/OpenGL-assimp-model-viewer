#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 vPosition;
out vec3 vNormal;

uniform mat4 viewModel;   
uniform mat4 proj;
uniform mat3 normalMat;  

void main() {
    vec4 viewPos = viewModel * vec4(aPos, 1.0);
    vPosition    = viewPos.xyz;
    vNormal      = normalize(normalMat * aNormal);
    gl_Position  = proj * viewPos;
}
