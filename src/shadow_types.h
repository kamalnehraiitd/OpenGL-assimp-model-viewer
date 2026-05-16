#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>


#define NUM_CASCADES 2
#define MAX_SHADOW_LIGHTS 4

struct ShadowCascade {
    GLuint    depthTex        = 0;
    glm::mat4 viewToLightClip = glm::mat4(1.0f);
};

struct LightShadowData {
    GLuint        fbo = 0;
    ShadowCascade cascades[NUM_CASCADES];
    float         splits[NUM_CASCADES];
    bool          initialized = false;
};