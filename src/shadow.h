#pragma once

#include "ui.h"
#include "shadow_types.h"
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include <limits>
#include "shaders.h"


inline std::vector<glm::vec4> getFrustumCornersCameraSpace(const glm::mat4& proj) {
    const glm::mat4 inv = glm::inverse(proj);
    std::vector<glm::vec4> corners;
    for (int x = 0; x < 2; x++)
        for (int y = 0; y < 2; y++)
            for (int z = 0; z < 2; z++) {
                glm::vec4 pt = inv * glm::vec4(
                    2.0f * x - 1.0f,
                    2.0f * y - 1.0f,
                    2.0f * z - 1.0f,
                    1.0f);
                corners.push_back(pt / pt.w);
            }
    return corners;
}



inline glm::mat4 buildLightMatrix(float nearPlane, float farPlane,
                                  const glm::vec3& camSpaceLightDir,
                                  float fov, float aspect,
                                  const glm::mat4& viewMat,
                                  std::vector<model>& scene,float padding,UIState& s,int cascadenum)
{

    glm::mat4 cascadeProj = glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
    std::vector<glm::vec4> corners = getFrustumCornersCameraSpace(cascadeProj);

  
    glm::vec3 center(0.0f);
    for (auto& v : corners) center += glm::vec3(v);
    center /= (float)corners.size();


    glm::vec3 dir = glm::normalize(camSpaceLightDir);
    glm::vec3 up  = glm::vec3(0.0f, 1.0f, 0.0f);
    if (abs(glm::dot(dir, up)) > 0.99f)
        up = glm::vec3(1.0f, 0.0f, 0.0f);


    glm::mat4 lightView = glm::lookAt(center - dir * 500.0f, center, up);

   
    float minX =  std::numeric_limits<float>::max();
    float maxX = -std::numeric_limits<float>::max();
    float minY =  std::numeric_limits<float>::max();
    float maxY = -std::numeric_limits<float>::max();
    float maxZ = -std::numeric_limits<float>::max();


    for (auto& v : corners) {
        glm::vec4 lv = lightView * v;
        minX = std::min(minX, lv.x); maxX = std::max(maxX, lv.x);
        minY = std::min(minY, lv.y); maxY = std::max(maxY, lv.y);
        maxZ = std::max(maxZ, -lv.z);  
    }
    float padX = (maxX - minX) * padding;
    float padY = (maxY - minY) * padding;
    minX -= padX;
    maxX += padX;
    minY -= padY;
    maxY += padY;

    float minZ = maxZ; 
    if(cascadenum==0){
        s.debugview=lightView;
    }
  
    for (int i = 0; i < (int)scene.size(); i++) {
        if (!scene[i].visible) continue;

        float sx = glm::length(glm::vec3(scene[i].models[0]));
        float sy = glm::length(glm::vec3(scene[i].models[1]));
        float sz = glm::length(glm::vec3(scene[i].models[2]));
        float maxScale = std::max({sx, sy, sz});
        float worldRadius = scene[i].radius * maxScale;

        glm::vec3 viewCentre = glm::vec3(viewMat * scene[i].models *
                                          glm::vec4(scene[i].centre, 1.0f));
        glm::vec4 lightCentre = lightView * glm::vec4(viewCentre, 1.0f);


        minZ = std::min(minZ, -lightCentre.z - worldRadius);

    }


    if (minZ >= maxZ) {
        minZ = maxZ - 1.0f;
    }

    glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    s.minz=minZ;
    s.maxz=maxZ;
    return lightProj * lightView;
}


inline void initShadowData(LightShadowData& sd, int resolution,
                           const float userSplits[NUM_CASCADES])
{
    if (sd.initialized) {
        glDeleteFramebuffers(1, &sd.fbo);
        for (int i = 0; i < NUM_CASCADES; i++)
            glDeleteTextures(1, &sd.cascades[i].depthTex);
    }

    glGenFramebuffers(1, &sd.fbo);

    for (int i = 0; i < NUM_CASCADES; i++) {
        glGenTextures(1, &sd.cascades[i].depthTex);
        glBindTexture(GL_TEXTURE_2D, sd.cascades[i].depthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, resolution, resolution,
                     0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    }
    glBindTexture(GL_TEXTURE_2D,0);//
    for (int i = 0; i < NUM_CASCADES; i++)
        sd.splits[i] = userSplits[i];

    sd.initialized = true;
    std::cout << "Shadow FBO: " << NUM_CASCADES
              << " cascades @ " << resolution << "\n";
}

inline void cleanupShadowData(LightShadowData& sd) {
    if (!sd.initialized) return;
    glDeleteFramebuffers(1, &sd.fbo);
    for (int i = 0; i < NUM_CASCADES; i++)
        glDeleteTextures(1, &sd.cascades[i].depthTex);
    sd.initialized = false;
}



struct model; 

inline void renderShadowMaps(LightShadowData& sd,
                             myshader& shadowShader,
                             const glm::vec3& camSpaceLightDir,
                             UIState& s,
                             std::vector<model>& scene,
                             int resolution)
{
    float aspect = (s.scrHeight > 0)
                 ? (float)s.scrWidth / (float)s.scrHeight
                 : 1.0f;


    float o = s.cascadeOverlap;
    float cascadeNears[NUM_CASCADES] = { s.cameraNear,        sd.splits[0] - o };
    float cascadeFars [NUM_CASCADES] = { sd.splits[0] + o,    sd.splits[1] };

    shadowShader.use();
    glViewport(0, 0, resolution, resolution);
    glBindFramebuffer(GL_FRAMEBUFFER, sd.fbo);
    glCullFace(GL_FRONT);

    for (int c = 0; c < NUM_CASCADES; c++) {

        sd.cascades[c].viewToLightClip = buildLightMatrix(
            cascadeNears[c], cascadeFars[c],
            camSpaceLightDir, s.fov, aspect,
            s.view, scene,s.padding,s,c);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               GL_TEXTURE_2D, sd.cascades[c].depthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glClear(GL_DEPTH_BUFFER_BIT);

        shadowShader.unifomrMat4("viewToLightClip", sd.cascades[c].viewToLightClip);

        for (int i = 0; i < (int)scene.size(); i++) {
            if (!scene[i].visible) continue;

            glm::mat4 viewModel = s.view * scene[i].models;
            shadowShader.unifomrMat4("viewModel", viewModel);

            for (int j = 0; j < (int)scene[i].meshes.size(); j++) {
              

                glBindVertexArray(scene[i].meshes[j].VAO);
                glDrawElements(GL_TRIANGLES, scene[i].meshes[j].indexcount,
                               GL_UNSIGNED_INT, 0);
                glBindVertexArray(0);
            }
        }
    }

    glCullFace(GL_BACK);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int fbW, fbH;
    glfwGetFramebufferSize(s.window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);
    glDrawBuffer(GL_BACK);
    glReadBuffer(GL_BACK);
}


// inline void uploadShadowUniforms(myshader& shader,
//                                  int shadowIndex,
//                                  LightShadowData& sd,
//                                  int baseTextureUnit)
// {
//     std::string p = "shadowData[" + std::to_string(shadowIndex) + "]";

//     for (int c = 0; c < NUM_CASCADES; c++) {
//         int unit = baseTextureUnit + c;
//         glActiveTexture(GL_TEXTUdRE0 + unit);
//         glBindTexture(GL_TEXTURE_2D, sd.cascades[c].depthTex);

//         int mapIdx = shadowIndex * NUM_CASCADES + c;
//         std::string samplerName = "shadowMaps[" + std::to_string(mapIdx) + "]";
//         shader.uniformi(samplerName.c_str(), unit);
//         shader.unifomrMat4((p + ".viewToLightClip[" + std::to_string(c) + "]").c_str(),
//                            sd.cascades[c].viewToLightClip);
//         shader.uniformf((p + ".splits[" + std::to_string(c) + "]").c_str(),
//                         sd.splits[c]);
//     }
  
//     shader.uniformi((p + ".hasShadow").c_str(), 1);
// }
inline void uploadShadowUniforms(myshader& shader,
                                 int shadowIndex,
                                 LightShadowData& sd,
                                 int baseTextureUnit)
{
    std::string p = "shadowData[" + std::to_string(shadowIndex) + "]";

    for (int c = 0; c < NUM_CASCADES; c++) {
        int unit = baseTextureUnit + c;
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, sd.cascades[c].depthTex);

        int mapIdx = shadowIndex * NUM_CASCADES + c;
        std::string samplerName = "shadowMaps[" + std::to_string(mapIdx) + "]";
        shader.uniformi(samplerName.c_str(), unit);
        shader.unifomrMat4((p + ".viewToLightClip[" + std::to_string(c) + "]").c_str(),
                           sd.cascades[c].viewToLightClip);
        shader.uniformf((p + ".splits[" + std::to_string(c) + "]").c_str(),
                        sd.splits[c]);
    }

    shader.uniformi((p + ".hasShadow").c_str(), 1);
}


inline void generateAllShadowMaps(UIState& s, myshader& shadowShader)
{
    if (!s.enableShadows || !s.sceneRef) return;

    std::vector<model>& scene = *s.sceneRef;
    int resolution = s.shadowMapRes;

    int count = 0;
    for (int i = 0; i < (int)s.lights.size(); i++) {
        if (!s.lights[i].enabled || !s.lights[i].castShadow) continue;
        if (count >= MAX_SHADOW_LIGHTS) break;

        if (!s.lights[i].shadowData.initialized)
            initShadowData(s.lights[i].shadowData, resolution, s.cascadeSplits);


        for (int c = 0; c < NUM_CASCADES; c++)
            s.lights[i].shadowData.splits[c] = s.cascadeSplits[c];

        glm::vec3 camDir;
        if (s.lights[i].space == 0) {
            camDir = glm::normalize(s.lights[i].direction);
        } else {
            camDir = glm::normalize(glm::mat3(s.view) * s.lights[i].direction);
        }

        renderShadowMaps(s.lights[i].shadowData, shadowShader,
                         camDir, s,
                         scene, resolution);
        count++;
    }
}

// inline void uploadAllShadowUniforms(UIState& s, myshader& shader)
// {
//     int baseUnit = 5;
//     int totalSlots = MAX_SHADOW_LIGHTS * NUM_CASCADES;  // 6

//     for (int i = 0; i < totalSlots; i++)
//         shader.uniformi(("shadowMaps[" + std::to_string(i) + "]").c_str(), baseUnit + i);

//     if (!s.enableShadows) {
//         for (int i = 0; i < MAX_SHADOW_LIGHTS; i++)
//             shader.uniformi(("shadowData[" + std::to_string(i) + "].hasShadow").c_str(), 0);
//         shader.uniformi("numShadowLights", 0);
//         glActiveTexture(GL_TEXTURE0);
//         return;
//     }

//     int shadowIdx = 0;
//     for (int i = 0; i < (int)s.lights.size(); i++) {
//         if (!s.lights[i].enabled || !s.lights[i].castShadow) continue;
//         if (shadowIdx >= MAX_SHADOW_LIGHTS) break;
//         if (!s.lights[i].shadowData.initialized) continue;

//         uploadShadowUniforms(shader, shadowIdx,
//                              s.lights[i].shadowData,
//                              baseUnit + shadowIdx * NUM_CASCADES);
//         shadowIdx++;
//     }

//     for (int i = shadowIdx; i < MAX_SHADOW_LIGHTS; i++)
//         shader.uniformi(("shadowData[" + std::to_string(i) + "].hasShadow").c_str(), 0);

//     shader.uniformi("numShadowLights", shadowIdx);

//     // Upload light direction for slope-based bias in shader
//     // Use the first shadow-casting light's direction in view space
//     for (int i = 0; i < (int)s.lights.size(); i++) {
//         if (!s.lights[i].enabled || !s.lights[i].castShadow) continue;
//         glm::vec3 camDir;
//         if (s.lights[i].space == 0)
//             camDir = glm::normalize(s.lights[i].direction);
//         else
//             camDir = glm::normalize(glm::mat3(s.view) * s.lights[i].direction);
//         shader.uniformVec3("shadowLightDir", camDir);
//         break;
//     }
//     shader.uniformi("debugcascades", s.shadowDebugVis ? 1 : 0);
//     shader.uniformi("PCF", s.PCF ? 1 : 0);
//     shader.uniformi("blend",s.blend);
//     glActiveTexture(GL_TEXTURE0);
// }
inline void uploadAllShadowUniforms(UIState& s, myshader& shader)
{
    int baseUnit = 5;
    int totalSlots = MAX_SHADOW_LIGHTS * NUM_CASCADES;

    for (int i = 0; i < totalSlots; i++)
        shader.uniformi(("shadowMaps[" + std::to_string(i) + "]").c_str(), baseUnit + i);

    if (!s.enableShadows) {
        for (int i = 0; i < MAX_SHADOW_LIGHTS; i++) {
            shader.uniformi(("shadowData[" + std::to_string(i) + "].hasShadow").c_str(), 0);
            shader.uniformi(("shadowLightIndex[" + std::to_string(i) + "]").c_str(), -1);
        }
        shader.uniformi("numShadowLights", 0);
        glActiveTexture(GL_TEXTURE0);
        return;
    }

    int shadowIdx = 0;
    int sceneLightIdx = 0;

    for (int i = 0; i < (int)s.lights.size(); i++) {
        if (!s.lights[i].enabled) continue;

        bool isShadow = s.lights[i].castShadow &&
                        s.lights[i].shadowData.initialized &&
                        shadowIdx < MAX_SHADOW_LIGHTS;

        if (isShadow) {
            uploadShadowUniforms(shader, shadowIdx,
                                 s.lights[i].shadowData,
                                 baseUnit + shadowIdx * NUM_CASCADES);

            shader.uniformi(("shadowLightIndex[" + std::to_string(shadowIdx) + "]").c_str(),
                            sceneLightIdx);

            glm::vec3 camDir;
            if (s.lights[i].space == 0)
                camDir = glm::normalize(s.lights[i].direction);
            else
                camDir = glm::normalize(glm::mat3(s.view) * s.lights[i].direction);
            shader.uniformVec3(("shadowLightDirs[" + std::to_string(shadowIdx) + "]").c_str(),
                               camDir);

            shadowIdx++;
        }

        sceneLightIdx++;
    }

    for (int i = shadowIdx; i < MAX_SHADOW_LIGHTS; i++) {
        shader.uniformi(("shadowData[" + std::to_string(i) + "].hasShadow").c_str(), 0);
        shader.uniformi(("shadowLightIndex[" + std::to_string(i) + "]").c_str(), -1);
    }

    shader.uniformi("numShadowLights", shadowIdx);
    shader.uniformi("debugcascades", s.shadowDebugVis ? 1 : 0);
    shader.uniformi("PCF", s.PCF ? 1 : 0);
    shader.uniformi("blend", s.blend);

    glActiveTexture(GL_TEXTURE0);
}
inline void cleanupAllShadowData(UIState& s) {
    for (auto& l : s.lights)
        cleanupShadowData(l.shadowData);
}