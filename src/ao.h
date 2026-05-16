#pragma once

#include "ui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <random>
#include <cmath>
#include "shaders.h"
#include "models.h"


static GLuint aoQuadVAO = 0, aoQuadVBO = 0;

inline void drawFullscreenQuad() {
    if (aoQuadVAO == 0) {
        float quad[] = {
            // pos              // uv
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
        };
        glGenVertexArrays(1, &aoQuadVAO);
        glGenBuffers(1, &aoQuadVBO);
        glBindVertexArray(aoQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, aoQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                              (void*)(3 * sizeof(float)));
        glBindVertexArray(0);
    }
    glBindVertexArray(aoQuadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}


inline void generateAOKernel(UIState& s) {
   
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

    s.aoKernel.clear();
    for (int i = 0; i < 64; ++i) {
  
        float u  = dist01(rng) * 2.0f - 1.0f;
        float v  = dist01(rng) * 2.0f - 1.0f;
        float w  = dist01(rng); 

        glm::vec3 pt = glm::normalize(glm::vec3(u, v, w));
      
        float t = (float)i / 63.0f;
        float reach = 0.05f + 0.95f * t * t; 
        pt *= dist01(rng) * reach;
        s.aoKernel.push_back(pt);
    }


    std::vector<glm::vec3> noiseData;
    for (int i = 0; i < 16; ++i) {
        float rx = dist01(rng) * 2.0f - 1.0f;
        float ry = dist01(rng) * 2.0f - 1.0f;
        noiseData.push_back(glm::vec3(rx, ry, 0.0f));
    }

    glGenTextures(1, &s.aoNoiseTex);
    glBindTexture(GL_TEXTURE_2D, s.aoNoiseTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0,
                 GL_RGB, GL_FLOAT, noiseData.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "AO: generated 64 kernel samples + 4x4 noise texture\n";
}

static inline void makeOrResizeTex(GLuint& tex, int w, int h, GLenum internalFmt,
                                    GLenum fmt, GLenum type) {
    if (!tex) glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

inline void buildAOBuffers(UIState& s, int w, int h) {

    if (!s.aoGbufferFBO) glGenFramebuffers(1, &s.aoGbufferFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.aoGbufferFBO);

    makeOrResizeTex(s.gPosition, w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, s.gPosition, 0);

    makeOrResizeTex(s.gNormal, w, h, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1,
                           GL_TEXTURE_2D, s.gNormal, 0);

    GLenum drawBufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBufs);

    if (!s.aoGbufferDepth) glGenRenderbuffers(1, &s.aoGbufferDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, s.aoGbufferDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, s.aoGbufferDepth);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "AO G-buffer FBO incomplete!\n";


    if (!s.aoFBO) glGenFramebuffers(1, &s.aoFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.aoFBO);

    makeOrResizeTex(s.aoColorBuffer, w, h, GL_RED, GL_RED, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, s.aoColorBuffer, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "AO result FBO incomplete!\n";

 
    if (!s.aoBlurFBO) glGenFramebuffers(1, &s.aoBlurFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.aoBlurFBO);

    makeOrResizeTex(s.aoColorBufferBlur, w, h, GL_RED, GL_RED, GL_FLOAT);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, s.aoColorBufferBlur, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "AO blur FBO incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // std::cout << "AO buffers: " << w << "x" << h << "\n";
}


inline void initAO(UIState& s) {
    int fbW, fbH;
    glfwGetFramebufferSize(s.window, &fbW, &fbH);

    buildAOBuffers(s, fbW, fbH);
    generateAOKernel(s);

   
    s.AOgeometry = new myshader("shaders/aogeo.vs",  "shaders/aogeo.fs");
    s.AOmain     = new myshader("shaders/aomain.vs", "shaders/aomain.fs");
    s.AOblur     = new myshader("shaders/aoblur.vs", "shaders/aoblur.fs");

    s.AOmain->use();
    s.AOmain->uniformi("gPosition", 0);
    s.AOmain->uniformi("gNormal",   1);
    s.AOmain->uniformi("noiseTex",  2);

    s.AOblur->use();
    s.AOblur->uniformi("aoInput", 0);

    std::cout << "AO: initialised\n";
}


inline void resizeAO(UIState& s, int fbW, int fbH) {
    if (fbW <= 0 || fbH <= 0) return;
    buildAOBuffers(s, fbW, fbH);
}


inline void aoGeometryPass(UIState& s, std::vector<model>& scene) {
    int fbW, fbH;
    glfwGetFramebufferSize(s.window, &fbW, &fbH);

    glBindFramebuffer(GL_FRAMEBUFFER, s.aoGbufferFBO);
    glViewport(0, 0, fbW, fbH);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    s.AOgeometry->use();
    s.AOgeometry->unifomrMat4("proj", s.projection);

    for (int i = 0; i < (int)scene.size(); ++i) {
        if (!scene[i].visible) continue;

        glm::mat4 vm  = s.view * scene[i].models;
        glm::mat3 nm  = glm::transpose(glm::inverse(glm::mat3(vm)));

        s.AOgeometry->unifomrMat4("viewModel", vm);
        s.AOgeometry->unifomrMat3("normalMat", nm);

        for (int j = 0; j < (int)scene[i].meshes.size(); ++j) {
            glBindVertexArray(scene[i].meshes[j].VAO);
            glDrawElements(GL_TRIANGLES, scene[i].meshes[j].indexcount,
                           GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void aoComputePass(UIState& s) {
    int fbW, fbH;
    glfwGetFramebufferSize(s.window, &fbW, &fbH);

    glBindFramebuffer(GL_FRAMEBUFFER, s.aoFBO);
    glViewport(0, 0, fbW, fbH);
    glClear(GL_COLOR_BUFFER_BIT);

    s.AOmain->use();


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s.gPosition);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, s.gNormal);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, s.aoNoiseTex);


    for (int i = 0; i < 64; ++i) {
        std::string name = "hemisphere[" + std::to_string(i) + "]";
        s.AOmain->uniformVec3(name.c_str(), s.aoKernel[i]);
    }

    s.AOmain->uniformi("sampleCount", 64);
    s.AOmain->uniformf("occlusionRadius", 0.5f);
    s.AOmain->uniformf("depthBias", 0.025f);
    s.AOmain->uniformVec2("framebufferSize", glm::vec2((float)fbW, (float)fbH));
    s.AOmain->unifomrMat4("proj", s.projection);

    glDisable(GL_DEPTH_TEST);
    drawFullscreenQuad();
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
inline void aoBlurPass(UIState& s) {
    int fbW, fbH;
    glfwGetFramebufferSize(s.window, &fbW, &fbH);

    glBindFramebuffer(GL_FRAMEBUFFER, s.aoBlurFBO);
    glViewport(0, 0, fbW, fbH);
    glClear(GL_COLOR_BUFFER_BIT);

    s.AOblur->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s.aoColorBuffer);

    glDisable(GL_DEPTH_TEST);
    drawFullscreenQuad();
    glEnable(GL_DEPTH_TEST);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
inline void executeAO(UIState& s, std::vector<model>& scene) {
    aoGeometryPass(s, scene);
    aoComputePass(s);
    aoBlurPass(s);
}
inline void bindAOTexture(UIState& s, myshader& shader) {
 
    int aoUnit = 4;
    glActiveTexture(GL_TEXTURE0 + aoUnit);
    glBindTexture(GL_TEXTURE_2D, s.aoColorBufferBlur);
    shader.uniformi("AOtexture", aoUnit);
    shader.uniformi("enableAO", s.enableAO ? 1 : 0);
    shader.uniformf("f_factor", s.aoFactor);
    shader.uniformi("debugAO", s.debugAO ? 1 : 0);
}

inline void cleanupAO(UIState& s) {
    if (s.aoGbufferFBO)     glDeleteFramebuffers(1, &s.aoGbufferFBO);
    if (s.aoFBO)            glDeleteFramebuffers(1, &s.aoFBO);
    if (s.aoBlurFBO)        glDeleteFramebuffers(1, &s.aoBlurFBO);
    if (s.gPosition)        glDeleteTextures(1, &s.gPosition);
    if (s.gNormal)          glDeleteTextures(1, &s.gNormal);
    if (s.aoColorBuffer)    glDeleteTextures(1, &s.aoColorBuffer);
    if (s.aoColorBufferBlur)glDeleteTextures(1, &s.aoColorBufferBlur);
    if (s.aoNoiseTex)       glDeleteTextures(1, &s.aoNoiseTex);
    if (s.aoGbufferDepth)   glDeleteRenderbuffers(1, &s.aoGbufferDepth);
    if (aoQuadVAO)          { glDeleteVertexArrays(1, &aoQuadVAO); glDeleteBuffers(1, &aoQuadVBO); }
    delete s.AOgeometry; delete s.AOmain; delete s.AOblur;
    s.AOgeometry = s.AOmain = s.AOblur = nullptr;
}