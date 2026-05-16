#pragma once 
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include<glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>


#include<iostream>
#include<fstream>
#include<string>
#include<sstream>


//took this function form learnopengl.com 
void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if(type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
///

class myshader
{
public:
    unsigned int ID;
    myshader(const char* vertexshaderpath,const char* fragshaderpath){
        unsigned int vertexshader;
        unsigned int fragmentgshader;
        vertexshader=glCreateShader(GL_VERTEX_SHADER);
        fragmentgshader=glCreateShader(GL_FRAGMENT_SHADER);
        std::ifstream file1(vertexshaderpath);
        std::ifstream file2(fragshaderpath);
        if(!file1.is_open()){
            std::cerr<<"could not load "<<vertexshaderpath<<"\n";
            exit(EXIT_FAILURE);
        }
        if(!file2.is_open()){
            std::cerr<<"could not load "<<fragshaderpath<<"\n";
            exit(EXIT_FAILURE);
        }
        std::stringstream buffer1,buffer2;
        buffer1<<file1.rdbuf();
        buffer2<<file2.rdbuf();
        file1.close();
        file2.close();

        std::string verprogram=buffer1.str();
        std::string fragprogram=buffer2.str();
        const char* verprogram_c=verprogram.c_str();
        const char* fragprogram_c=fragprogram.c_str();
        glShaderSource(vertexshader,1,&verprogram_c,NULL);
        glShaderSource(fragmentgshader,1,&fragprogram_c,NULL);
        glCompileShader(vertexshader);
        glCompileShader(fragmentgshader);
        checkCompileErrors(vertexshader,"Vertex");
        checkCompileErrors(fragmentgshader,"Fragment");

        ID=glCreateProgram();
        glAttachShader(ID,vertexshader);
        glAttachShader(ID,fragmentgshader);
        glLinkProgram(ID);
        GLint success;
        glGetProgramiv(ID, GL_LINK_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetProgramInfoLog(ID, 1024, NULL, log);
            std::cerr << "SHADER LINK ERROR:\n" << log << std::endl;
        }
        checkCompileErrors(ID,"PROGRAM");
        glDeleteShader(vertexshader);
        glDeleteShader(fragmentgshader);
    }
    void use(){
        glUseProgram(ID);
    }

    ~myshader(){
        glDeleteProgram(ID);
    }
    void uniformi(const char* s,int value){
        glUniform1i(glGetUniformLocation(ID, s), value); 
    }
    void uniformf(const char* s,float value){
        glUniform1f(glGetUniformLocation(ID, s), value); 
    }
    void uniformVec2(const char* s,const glm::vec2& value){
        glUniform2fv(glGetUniformLocation(ID, s),1,glm::value_ptr(value) ); 
    }
    void uniformVec2(const char* s,float x,float y){
        glUniform2f(glGetUniformLocation(ID, s),x,y ); 
    }
    void uniformVec3(const char* s,const glm::vec3& value){
        glUniform3fv(glGetUniformLocation(ID, s),1,glm::value_ptr(value) ); 
    }
    void uniformVec3(const char* s,float x,float y,float z){
        glUniform3f(glGetUniformLocation(ID, s),x,y,z ); 
    }
    void uniformVec4(const char* s,const glm::vec4& value){
        glUniform4fv(glGetUniformLocation(ID, s),1,glm::value_ptr(value) ); 
    }
    void uniformVec4(const char* s,float x,float y,float z,float w){
        glUniform4f(glGetUniformLocation(ID, s),x,y,z,w ); 
    }
    void unifomrMat2(const char* s, const glm::mat2 &mat)
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, s), 1, GL_FALSE, glm::value_ptr(mat));
    }
    void unifomrMat3(const char* s, const glm::mat3 &mat)
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, s), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void unifomrMat4(const char* s, const glm::mat4 &mat)
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, s), 1, GL_FALSE, glm::value_ptr(mat));
    }  
};
























































