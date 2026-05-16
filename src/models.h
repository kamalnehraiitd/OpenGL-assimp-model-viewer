#pragma once 

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include<stb_image.h>
#include <iostream>
#include <string>
#include <vector>
#include "shaders.h"
#include <limits>



GLuint loadtexture(std::string finalpath){  
    GLuint ID;
    glGenTextures(1, &ID);
    int w, h, n;

    unsigned char *data = stbi_load(finalpath.c_str(), &w, &h, &n, 0);
    if (data)
    {
        GLenum format;
        if (n == 1)      format = GL_RED;
        else if (n == 3) format = GL_RGB;
        else if (n == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, ID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        std::cout << "    [TEX OK] " << finalpath << " (" << w << "x" << h << ", " << n << " ch)\n";
        stbi_image_free(data);
    }
    else
    {
        std::cout << "    [TEX FAIL] " << finalpath << "\n";
        stbi_image_free(data);
    }
    return ID;
}


struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 Texcords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};


struct Texture {
    GLuint id;
    int type;
};

struct MaterialData {
    std::string name;

    glm::vec3 Kd = glm::vec3(0.6f);
    glm::vec3 Ks = glm::vec3(0.0f);
    glm::vec3 Ka = glm::vec3(0.1f);
    float Ns     = 32.0f;
    float Ni     = 1.5f;
    float d      = 1.0f;
    float rough  = 0.5f;


    glm::vec3 Kd_orig = glm::vec3(0.6f);
    glm::vec3 Ks_orig = glm::vec3(0.0f);
    glm::vec3 Ka_orig = glm::vec3(0.1f);
    float Ns_orig     = 32.0f;
    float Ni_orig     = 1.5f;
    float d_orig      = 1.0f;
    float rough_orig  = 0.5f;

  
    bool hasDiffuseMap  = false;
    bool hasSpecularMap = false;
    bool hasAmbientMap  = false;
    bool hasNormalMap   = false;
    bool hasOpacityMap  = false;

   
    bool useDiffuseMap  = false;
    bool useSpecularMap = false;
    bool useAmbientMap  = false;
    bool useNormalMap   = false;
    bool useOpacityMap  = false;
    bool flipNormalY = false;
    void resetToOriginal() {
        Kd    = Kd_orig;
        Ks    = Ks_orig;
        Ka    = Ka_orig;
        Ns    = Ns_orig;
        Ni    = Ni_orig;
        d     = d_orig;
        rough = rough_orig;
        useDiffuseMap  = hasDiffuseMap;
        useSpecularMap = hasSpecularMap;
        useAmbientMap  = hasAmbientMap;
        useNormalMap   = hasNormalMap;
        useOpacityMap  = hasOpacityMap;
    }

};


class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    GLuint VAO, VBO, EBO;
    unsigned int indexcount;
    MaterialData material;
    bool iscooktorrence;
    bool hasTangents;
    unsigned int materialIndex;  

    Mesh(aiMesh* mesh, const aiScene* scene, std::string path) {
        iscooktorrence = false;
        hasTangents = false;
        materialIndex = mesh->mMaterialIndex;

        std::cout << "  ── Loading mesh: \"" << mesh->mName.C_Str()
                  << "\" (" << mesh->mNumVertices << " verts, "
                  << mesh->mNumFaces << " faces, matIdx=" << materialIndex << ")\n";


        vertices.reserve(mesh->mNumVertices);
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            vertex.Position = glm::vec3(mesh->mVertices[i].x,
                                        mesh->mVertices[i].y,
                                        mesh->mVertices[i].z);

            if (mesh->mNormals)
                vertex.Normal = glm::vec3(mesh->mNormals[i].x,
                                          mesh->mNormals[i].y,
                                          mesh->mNormals[i].z);
            else
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);

            if (mesh->mTextureCoords[0])
                vertex.Texcords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                            mesh->mTextureCoords[0][i].y);
            else
                vertex.Texcords = glm::vec2(0.0f);

            if (mesh->mTangents) {
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x,
                                           mesh->mTangents[i].y,
                                           mesh->mTangents[i].z);
                vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x,
                                              mesh->mBitangents[i].y,
                                              mesh->mBitangents[i].z);
            } else {
                vertex.Tangent = glm::vec3(0.0f);
                vertex.Bitangent = glm::vec3(0.0f);
            }
            vertices.push_back(vertex);
        }
        hasTangents = (mesh->mTangents != nullptr);


        indices.reserve(mesh->mNumFaces * 3);
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
            for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; j++)
                indices.push_back(mesh->mFaces[i].mIndices[j]);

        std::cout << "    normals=" << (mesh->mNormals ? "Y" : "N")
                  << " UVs=" << (mesh->mTextureCoords[0] ? "Y" : "N")
                  << " tangents=" << (hasTangents ? "Y" : "N") << "\n";

     
        aiMaterial* mat = scene->mMaterials[mesh->mMaterialIndex];
        loadMaterial(mat);
        loadTextures(mat, path);
        
      
        setupVAO();
        indexcount = indices.size();
    }

private:
    void loadMaterial(aiMaterial* mat) {
        aiString matName;
        if (mat->Get(AI_MATKEY_NAME, matName) == AI_SUCCESS)
            material.name = matName.C_Str();
        else
            material.name = "unnamed";
        std::cout << "    Material: \"" << material.name << "\"\n";

        ai_int32 illum = -1;
        if (mat->Get(AI_MATKEY_SHADING_MODEL, illum) == AI_SUCCESS) {
            std::cout << "    illum: " << illum;
            if (illum == 2) { iscooktorrence = true; std::cout << " → Cook-Torrance\n"; }
            else { std::cout << " → Blinn-Phong\n"; }
        } else {
            iscooktorrence = false;
            std::cout << "    illum: NOT FOUND → Blinn-Phong\n";
        }

        aiColor3D color;
        if (mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            material.Kd = glm::vec3(color.r, color.g, color.b);
        else material.Kd = glm::vec3(0.6f);

        if (mat->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS)
            material.Ka = glm::vec3(color.r, color.g, color.b);
        else material.Ka = glm::vec3(0.1f);

        if (mat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
            material.Ks = glm::vec3(color.r, color.g, color.b);
        else material.Ks = glm::vec3(0.0f);

        float fval;
        if (mat->Get(AI_MATKEY_SHININESS, fval) == AI_SUCCESS)  material.Ns = fval;
        else material.Ns = 32.0f;
        if (mat->Get(AI_MATKEY_REFRACTI, fval) == AI_SUCCESS)   material.Ni = fval;
        else material.Ni = 1.5f;
        if (mat->Get(AI_MATKEY_OPACITY, fval) == AI_SUCCESS)    material.d = fval;
        else material.d = 1.0f;

        if (iscooktorrence) {
            float roughness = 0.0f;
            if (mat->Get("$mat.roughnessFactor", 0, 0, roughness) == AI_SUCCESS)
                material.rough = roughness;
            else material.rough = 0.5f;
        } else {
            material.rough = 0.5f;
        }


        material.Kd_orig = material.Kd; material.Ks_orig = material.Ks;
        material.Ka_orig = material.Ka; material.Ns_orig = material.Ns;
        material.Ni_orig = material.Ni; material.d_orig  = material.d;
        material.rough_orig = material.rough;

        std::cout << "    Kd=(" << material.Kd.x << "," << material.Kd.y << "," << material.Kd.z << ")"
                  << " Ks=(" << material.Ks.x << "," << material.Ks.y << "," << material.Ks.z << ")"
                  << " Ns=" << material.Ns << " Ni=" << material.Ni
                  << " d=" << material.d << " rough=" << material.rough << "\n";
    }

    void loadTextures(aiMaterial* mat, const std::string& path) {

        for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++) {
            aiString str; mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
            Texture t; t.id = loadtexture(path + "/" + str.C_Str()); t.type = 0;
            textures.push_back(t);
            material.hasDiffuseMap = true; material.useDiffuseMap = true;
        }

        for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_SPECULAR); i++) {
            aiString str; mat->GetTexture(aiTextureType_SPECULAR, i, &str);
            Texture t; t.id = loadtexture(path + "/" + str.C_Str()); t.type = 1;
            textures.push_back(t);
            material.hasSpecularMap = true; material.useSpecularMap = true;
        }

        for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_AMBIENT); i++) {
            aiString str; mat->GetTexture(aiTextureType_AMBIENT, i, &str);
            Texture t; t.id = loadtexture(path + "/" + str.C_Str()); t.type = 2;
            textures.push_back(t);
            material.hasAmbientMap = true; material.useAmbientMap = true;
        }
      
        for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_NORMALS); i++) {
            aiString str; mat->GetTexture(aiTextureType_NORMALS, i, &str);
            Texture t; t.id = loadtexture(path + "/" + str.C_Str()); t.type = 3;
            textures.push_back(t);
            material.hasNormalMap = true; material.useNormalMap = true;
        }
        if (!material.hasNormalMap) {
            for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_HEIGHT); i++) {
                aiString str; mat->GetTexture(aiTextureType_HEIGHT, i, &str);
                Texture t; t.id = loadtexture(path + "/" + str.C_Str()); t.type = 3;
                textures.push_back(t);
                material.hasNormalMap = true; material.useNormalMap = true;
            }
        }
  
        for (unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_OPACITY); i++) {
            aiString str; mat->GetTexture(aiTextureType_OPACITY, i, &str);
            Texture t; t.id = loadtexture(path + "/" + str.C_Str()); t.type = 4;
            textures.push_back(t);
            material.hasOpacityMap = true; material.useOpacityMap = true;
        }

        std::cout << "    Textures: " << textures.size()
                  << " (d=" << material.hasDiffuseMap << " s=" << material.hasSpecularMap
                  << " a=" << material.hasAmbientMap << " n=" << material.hasNormalMap
                  << " o=" << material.hasOpacityMap << ")\n";
    }

    void setupVAO() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Texcords));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
        glBindVertexArray(0);
    }

public:
    void drawmesh(myshader& shader, glm::mat4& model, int selection) {
        shader.use();
        shader.unifomrMat4("model", model);
        shader.uniformVec3("material.Kd", material.Kd);
        shader.uniformVec3("material.Ks", material.Ks);
        shader.uniformVec3("material.Ka", material.Ka);
        shader.uniformf("material.Ns",    material.Ns);
        shader.uniformf("material.Ni",    material.Ni);
        shader.uniformf("material.d",     material.d);
        shader.uniformi("iscooktorrence", iscooktorrence);
        if (iscooktorrence)
            shader.uniformf("material.roughness", material.rough);

        shader.uniformi("hasDiffuseMap",  material.useDiffuseMap  ? 1 : 0);
        shader.uniformi("hasSpecularMap", material.useSpecularMap ? 1 : 0);
        shader.uniformi("hasAmbientMap",  material.useAmbientMap  ? 1 : 0);
        shader.uniformi("hasNormalMap",   material.useNormalMap   ? 1 : 0);
        shader.uniformi("flipNormalY", material.flipNormalY ? 1 : 0);
        for (int i = 0; i < (int)textures.size(); i++) {
            if (textures[i].type == 0 && material.useDiffuseMap) {
                glActiveTexture(GL_TEXTURE0); shader.uniformi("material.diffuseMap", 0);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            } else if (textures[i].type == 1 && material.useSpecularMap) {
                glActiveTexture(GL_TEXTURE1); shader.uniformi("material.specularMap", 1);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            } else if (textures[i].type == 2 && material.useAmbientMap) {
                glActiveTexture(GL_TEXTURE2); shader.uniformi("material.ambientMap", 2);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            } else if (textures[i].type == 3 && material.useNormalMap) {
                glActiveTexture(GL_TEXTURE3); shader.uniformi("material.normalMap", 3);
                glBindTexture(GL_TEXTURE_2D, textures[i].id);
            } 
        }

        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexcount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};


class model {
public:
    std::vector<Mesh> meshes;
    std::string directoy;
    glm::mat4 models;

    glm::vec3 centre=glm::vec3(0.0f);
    std::string name;
    float radius=0;
    int selection;
    bool visible;

    model(const std::string path, const std::string dir) {
        directoy = dir; selection = 0; visible = true;

        radius = 0.0f; centre = glm::vec3(0.0f); models = glm::mat4(1.0f); 

        unsigned int flags = aiProcess_Triangulate | aiProcess_FlipUVs
                           | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices
                           | aiProcess_CalcTangentSpace;
        Assimp::Importer imp;
        const aiScene* scene = imp.ReadFile(path, flags);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "ERROR::ASSIMP::" << imp.GetErrorString() << std::endl;
            exit(EXIT_FAILURE);
        }
        std::cout << "\n══ Loading: " << path << " (meshes=" << scene->mNumMeshes
                  << " materials=" << scene->mNumMaterials << ") ══\n";
        name = path;
        for (unsigned int i = 0; i < scene->mNumMeshes; i++)
            meshes.emplace_back(scene->mMeshes[i], scene, directoy);

        int numver = 0;
        for (auto& m : meshes) { for (auto& v : m.vertices) centre += v.Position; numver += m.vertices.size(); }
        if (numver > 0) centre /= (float)numver;
        for (auto& m : meshes) for (auto& v : m.vertices)
            radius = std::max(radius, glm::length(v.Position - centre));
        if(path=="a2_models/platform.obj"){
            radius=0;
        }
        std::cout << "  Centre=(" << centre.x << "," << centre.y << "," << centre.z
                  << ") Radius=" << radius << "\n══ Done ══\n\n";
        imp.FreeScene();
    }

    void draw(myshader& Shader) {
        if (!visible) return;
        for (auto& m : meshes) m.drawmesh(Shader, models, selection);
    }


    std::vector<unsigned int> getUniqueMaterialIndices() const {
        std::vector<unsigned int> indices;
        for (const auto& m : meshes) {
            bool found = false;
            for (auto idx : indices) if (idx == m.materialIndex) { found = true; break; }
            if (!found) indices.push_back(m.materialIndex);
        }
        return indices;
    }

  
    std::vector<int> getMeshesForMaterial(unsigned int matIdx) const {
        std::vector<int> result;
        for (int i = 0; i < (int)meshes.size(); i++)
            if (meshes[i].materialIndex == matIdx) result.push_back(i);
        return result;
    }
};




