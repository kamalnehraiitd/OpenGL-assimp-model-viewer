#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main() {

    vec4 worldPos = model * vec4(aPos, 1.0);

    vec3 worldNormal = normalize(mat3(transpose(inverse(model))) * aNormal);



    worldPos.xyz += worldNormal * 0.05;


    gl_Position = proj * view * worldPos;
}