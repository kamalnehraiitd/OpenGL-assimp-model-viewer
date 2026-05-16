#version 410 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
out mat3 TBN;
out vec4 ClipPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    TexCoords = aTexCoords;

    mat4 viewModel = view * model;
    FragPos = vec3(viewModel * vec4(aPos, 1.0));


    mat3 normalMatrix = mat3(transpose(inverse(viewModel)));

    Normal = normalize(normalMatrix * aNormal);

  
    vec3 T = normalize(normalMatrix * aTangent);
    vec3 N = Normal;
    T = normalize(T - dot(T, N) * N);  
    vec3 B = cross(N, T);               

    TBN = mat3(T, B, N);

    gl_Position = proj * vec4(FragPos, 1.0);
    ClipPos=gl_Position;
}
