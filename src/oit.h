#pragma once

#include "ui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include "shaders.h"

inline void buildOITBuffers(UIState& s, int w, int h) {
    if (!s.oitFBO) glGenFramebuffers(1, &s.oitFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.oitFBO);

    if (!s.oitAccumTex) glGenTextures(1, &s.oitAccumTex);
    glBindTexture(GL_TEXTURE_2D, s.oitAccumTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s.oitAccumTex, 0);

    if (!s.oitRevealTex) glGenTextures(1, &s.oitRevealTex);
    glBindTexture(GL_TEXTURE_2D, s.oitRevealTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, s.oitRevealTex, 0);

    GLenum drawBufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, drawBufs);

    if (!s.oitDepthRBO) glGenRenderbuffers(1, &s.oitDepthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, s.oitDepthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s.oitDepthRBO);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Weighted OIT FBO incomplete!\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void initOIT(UIState& s) {
    int fbW, fbH;
    glfwGetFramebufferSize(s.window, &fbW, &fbH);
    buildOITBuffers(s, fbW, fbH);
    s.OITcomposite = new myshader("shaders/oit.vs", "shaders/oit.fs");
}

inline void resizeOIT(UIState& s, int fbW, int fbH) {
    if (fbW <= 0 || fbH <= 0) return;
    buildOITBuffers(s, fbW, fbH);
}

inline void weightedOIT_begin(UIState& s, int fbW, int fbH) {
    GLuint srcFBO = s.enableAA ? s.msaaFBO : 0;
    glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, s.oitFBO);
    glBlitFramebuffer(0, 0, fbW, fbH, 0, 0, fbW, fbH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, s.oitFBO);
    glViewport(0, 0, fbW, fbH);
    glClearBufferfv(GL_COLOR, 0, glm::value_ptr(glm::vec4(0.0f)));
    float one = 1.0f;
    glClearBufferfv(GL_COLOR, 1, &one);

    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunci(0, GL_ONE, GL_ONE);
    glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    glBlendEquation(GL_FUNC_ADD);
}

inline void weightedOIT_end(UIState& s, int fbW, int fbH) {
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    if (s.enableAA)
        glBindFramebuffer(GL_FRAMEBUFFER, s.msaaFBO);
    else
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbW, fbH);

    glDepthFunc(GL_ALWAYS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    s.OITcomposite->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s.oitAccumTex);
    s.OITcomposite->uniformi("accumTex", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, s.oitRevealTex);
    s.OITcomposite->uniformi("revealTex", 1);

    drawFullscreenQuad();

    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

inline void cleanupOIT(UIState& s) {
    if (s.oitFBO)       glDeleteFramebuffers(1, &s.oitFBO);
    if (s.oitAccumTex)  glDeleteTextures(1, &s.oitAccumTex);
    if (s.oitRevealTex) glDeleteTextures(1, &s.oitRevealTex);
    if (s.oitDepthRBO)  glDeleteRenderbuffers(1, &s.oitDepthRBO);
    delete s.OITcomposite; s.OITcomposite = nullptr;
}