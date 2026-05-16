#version 410 core

layout (location = 0) in vec3 aPos;
uniform mat4 view;
uniform mat4 model;
uniform mat4 proj;

void main()
{
    mat4 viewModel = view;
    vec3 FragPos = vec3(viewModel * vec4(aPos, 1.0));
    gl_Position = proj * vec4(FragPos, 1.0);
}


