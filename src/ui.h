#pragma once

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <iostream>
#include "shadow_types.h" 
#include "shaders.h"     
struct GLFWwindow;
struct model;
class Mesh;
struct MaterialData;
struct Camera {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 front    = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up       = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right    = glm::vec3(1.0f, 0.0f, 0.0f);
    float yaw          = -90.0f;
    float pitch        = 0.0f;
    float distfactor   = 1.0f;
};
struct DirLightData {
    glm::vec3 direction;
    glm::vec3 color;
    bool      enabled        = true;
    int       space          = 0;       
    bool      castShadow     = true;
    int       activeCascades = NUM_CASCADES;   
    glm::mat4 lightspacematrix;
    LightShadowData shadowData;                
};
struct UIState {
   
    std::string  dirpath;
    unsigned int scrWidth   = 1000;
    unsigned int scrHeight  = 800;
    int          fbWidth    = 0;
    int          fbHeight   = 0;
    float        deltatime  = 0.0f;
    float        fov        = 45.0f;
    GLFWwindow* window;
    Camera    camera;
    glm::vec3 worldup     = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 projection  = glm::mat4(1.0f);
    glm::mat4 view        = glm::mat4(1.0f);
    bool      firstMouse  = false;
    float     lastx       = 500.0f;
    float     lasty       = 400.0f;
    glm::vec3 trackballTarget = glm::vec3(0.0f);
    glm::vec3 trackballEye    = glm::vec3(0.0f, 0.0f, 2.0f);
    glm::vec3 trackballUp     = glm::vec3(0.0f, 1.0f, 0.0f);
    float trackballDistance    = 2.0f;
    std::vector<DirLightData> lights;
    model*      selectedobj     = nullptr;
    int         selectedindex   =-1;
    int         hoveredIndex    = -1;
    bool        capsLockPressed = false;
    std::string selected        = "none";
    std::string selectedname;
    bool trackball     = true;
    bool cursorLocked  = true;
    bool cubehighlight = false;
    bool showGrid      = true;
    bool isMouseDown = false;
    bool isDragging = false;
    double clickStartX = 0.0;
    double clickStartY = 0.0;
    const float DRAG_THRESHOLD = 5.0f; 
     float fps          = 0.0f;
    float frametime_ms = 0.0f;
    bool  hasSelection = false;
    int   sceneObjectCount = 0;
    GLuint pickFBO   = 0;
    GLuint pickColor = 0;
    GLuint pickDepth = 0;
    GLuint crossVAO = 0;
    GLuint crossVBO = 0;
    std::vector<model>* sceneRef = nullptr;
    float cookambientStrength = 0.1f;
    float blinambientStrength = 0.1f;
    bool enableAA       = false;
    int  aaSamplesX     = 2;
    int  aaSamplesY     = 2;
    bool aaNeedsRestart = false;
    int  aaMethod       = 0;
    bool  enableShadows    = false;
    int   shadowMapRes     = 2048;
    float cameraNear       = 0.1f;
    float cameraFar        = 100.0f;
    float cascadeSplits[NUM_CASCADES] = { 30.0f, 100.0f };  
    float shadowSplit      = 30.0f;      
    bool  shadowDebugVis   = false;      
    bool  enableAO  = false;
    float aoFactor  = 1.0f;
    bool  debugAO   = false;
    bool scaleEntireScene=true;
    GLuint aoGbufferFBO     = 0;
    GLuint aoGbufferDepth   = 0;
    GLuint gPosition        = 0;
    GLuint gNormal          = 0;
    GLuint aoFBO            = 0;
    GLuint aoBlurFBO        = 0;
    GLuint aoColorBuffer    = 0;
    GLuint aoColorBufferBlur= 0;
    GLuint aoNoiseTex       = 0;
    std::vector<glm::vec3> aoKernel;
    myshader* AOgeometry = nullptr;
    myshader* AOmain     = nullptr;
    myshader* AOblur     = nullptr;


    
    bool enableOIT = true;
    bool takeScreenshot = false;

    GLuint oitFBO            = 0;
    GLuint oitAccumTex       = 0;   
    GLuint oitRevealTex      = 0;  
    GLuint oitDepthRBO       = 0;   
    myshader* OITcomposite   = nullptr;

    GLuint dpPeelFBO        = 0;
    GLuint dpAccumFBO       = 0;
    GLuint dpCopyFBO        = 0;
    GLuint dpDepthTex[2]    = {0, 0};   
    GLuint dpColorTex       = 0;       
    GLuint dpAccumTex       = 0;      
    GLuint dpOpaqueDepthTex = 0;
    int    dpMaxPeels       = 6;
    int    oitMethod        = 0;       
    myshader* dpBlendShader  = nullptr;
    myshader* dpFinalShader  = nullptr;
   
   
   
   
   
   
   
   
    GLuint msaaFBO   = 0;
    GLuint msaaColor = 0;
    GLuint msaaDepth = 0;


    bool  cascadeFrozen = false;
    int   frozenCascadeCount = 0;
    glm::mat4 frozenLightMatrices[NUM_CASCADES];
    GLuint cascadeDebugVAO = 0;
    GLuint cascadeDebugVBO = 0;
    float cascadeOverlap = 5.0f;
    float padding=0.1;
    float minz=0;
    float maxz=0;
    glm::mat4 debugview;
    bool PCF=true;
    int blend=1;

};


inline void updateCameraVectors(UIState& s) {
    glm::vec3 f;
    f.x = cos(glm::radians(s.camera.yaw)) * cos(glm::radians(s.camera.pitch));
    f.y = sin(glm::radians(s.camera.pitch));
    f.z = sin(glm::radians(s.camera.yaw)) * cos(glm::radians(s.camera.pitch));
    s.camera.front = glm::normalize(f);
    s.camera.right = glm::normalize(glm::cross(s.camera.front, s.worldup));
    s.camera.up    = glm::normalize(glm::cross(s.camera.right, s.camera.front));
}


inline void initPickFBO(UIState& s, GLFWwindow* window) {
    glfwGetFramebufferSize(window, &s.fbWidth, &s.fbHeight);
    glGenFramebuffers(1, &s.pickFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.pickFBO);
    glGenTextures(1, &s.pickColor);
    glBindTexture(GL_TEXTURE_2D, s.pickColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, s.fbWidth, s.fbHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s.pickColor, 0);
    glGenRenderbuffers(1, &s.pickDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, s.pickDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, s.fbWidth, s.fbHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s.pickDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "Pick FBO not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

inline void resizePickFBO(UIState& s, int fbW, int fbH) {
    s.fbWidth = fbW; s.fbHeight = fbH;
    glBindTexture(GL_TEXTURE_2D, s.pickColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbW, fbH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, s.pickDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, fbW, fbH);
}


inline void initCrosshair(UIState& s) {
    float verts[] = { -0.02f, 0.0f, 0.02f, 0.0f, 0.0f, -0.02f, 0.0f, 0.02f };
    glGenVertexArrays(1, &s.crossVAO); glGenBuffers(1, &s.crossVBO);
    glBindVertexArray(s.crossVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s.crossVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}


inline void UI_Init(GLFWwindow* window, const char* glsl_version) {
    IMGUI_CHECKVERSION(); ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::GetStyle().WindowRounding = 0.0f;
        ImGui::GetStyle().Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
}

inline void UI_Shutdown() {
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
}

inline void UI_NewFrame() {
    ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
}

inline void UI_SyncState(UIState& s) {
    s.fps = ImGui::GetIO().Framerate;
    s.frametime_ms = 1000.0f / s.fps;
    s.hasSelection = (s.selectedindex!=-1);
}


inline void UI_DrawScenePanel(UIState& s) {
    ImGui::Begin("Renderer");


    ImGui::Text("%.1f FPS  (%.3f ms)", s.fps, s.frametime_ms);
    ImGui::Separator();


    ImGui::Text("Camera pos  (%.2f, %.2f, %.2f)",
                s.camera.position.x, s.camera.position.y, s.camera.position.z);
    ImGui::Text("Yaw %.1f  Pitch %.1f", s.camera.yaw, s.camera.pitch);
    ImGui::Checkbox("Trackball mode", &s.trackball);
    ImGui::Checkbox("Show grid (G)", &s.showGrid);
    ImGui::Separator();
    ImGui::Text("Selected: %s", s.hasSelection ? "yes" : "none");
    ImGui::Text("Hovered:  %s", s.selected.c_str());
    ImGui::Text("Objects:  %d", s.sceneObjectCount);
    ImGui::Text("selcted objct %d",s.selectedindex);
    ImGui::Separator();

  
    ImGui::SliderFloat("Blinn ambient", &s.blinambientStrength, 0.0f, 1.0f);
    ImGui::SliderFloat("Cook ambient",  &s.cookambientStrength, 0.0f, 1.0f);
    ImGui::Separator();


    ImGui::Checkbox("Anti-Aliasing (MSAA)", &s.enableAA);
    if (s.enableAA) {
        int maxSamples;
        glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
        bool changed = false;
        if (ImGui::SliderInt("AA Samples", &s.aaSamplesX, 1, maxSamples)) {
            s.aaSamplesY = s.aaSamplesX;
            changed = true;
        }
        if (changed) s.aaNeedsRestart = true;
        ImGui::Text("Samples: %d (max %d)", s.aaSamplesX, maxSamples);
        if (s.aaNeedsRestart)
            ImGui::TextColored(ImVec4(1,1,0,1), "Rebuild needed");
        
    }
    ImGui::Separator();


    ImGui::Checkbox("Shadows", &s.enableShadows);
    if (s.enableShadows) {
  
        if (ImGui::SliderFloat("Cascade Split", &s.shadowSplit, 5.0f, 90.0f)) {
            s.cascadeSplits[0] = s.shadowSplit;
            s.cascadeSplits[1] = s.cameraFar;
        }
        ImGui::Text("Cascade 0: %.1f - %.1f", s.cameraNear, s.shadowSplit);
        ImGui::Text("Cascade 1: %.1f - %.1f", s.shadowSplit, s.cameraFar);
        ImGui::SliderFloat("Cascade Overlap", &s.cascadeOverlap, 0.0f, 15.0f);
        ImGui::SliderFloat("padding", &s.padding, 0.0f, 1.0f);
  
        ImGui::Checkbox("Visualize cascades", &s.shadowDebugVis);
        ImGui::Checkbox("PCF", &s.PCF);
        ImGui::SliderInt("blend",&s.blend,0,2);
        if(s.shadowDebugVis){
            ImGui::Text("minZ %.1f maxZ %.1f",s.minz,s.maxz);
        }}
        ImGui::Checkbox("Ambient Occlusion", &s.enableAO);
        if (s.enableAO) {
            ImGui::SliderFloat("AO factor", &s.aoFactor, 0.0f, 2.0f);
            ImGui::Checkbox("Debug AO", &s.debugAO);
            ImGui::Checkbox("Scale entire scene", &s.scaleEntireScene);
        }
        ImGui::Separator();

   
        ImGui::Checkbox("OIT Transparency", &s.enableOIT);

        ImGui::End();
    
}

inline void UI_DrawLightPanel(UIState& s) {
    
    ImGui::Begin("Lights");
    ImGui::Text("Total: %d", (int)s.lights.size());
    ImGui::Separator();

    if (ImGui::Button("+ Add Light")) {
        DirLightData nl;
        nl.direction      = glm::vec3(0.0f, -1.0f, 0.0f);
        nl.color          = glm::vec3(1.0f);
        nl.enabled        = true;
        nl.space          = 0;
        nl.castShadow     = true;
        nl.activeCascades = NUM_CASCADES;
        s.lights.push_back(nl);
    }
    ImGui::Separator();

    int deleteIdx = -1;
    for (int i = 0; i < (int)s.lights.size(); i++) {
        ImGui::PushID(i);
        DirLightData& l = s.lights[i];
        const char* sp = (l.space == 0) ? "Cam" : "World";

        bool open = ImGui::TreeNode("", "Light %d [%s]", i, sp);

        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        ImGui::Checkbox("On", &l.enabled);

        if (open) {
          
            ImGui::RadioButton("Camera", &l.space, 0);
            ImGui::SameLine();
            ImGui::RadioButton("World", &l.space, 1);

      
            float dir[3] = {l.direction.x, l.direction.y, l.direction.z};
            if (ImGui::SliderFloat3("Dir", dir, -1.0f, 1.0f))
                l.direction = glm::vec3(dir[0], dir[1], dir[2]);

   
            float col[3] = {l.color.x, l.color.y, l.color.z};
            if (ImGui::ColorEdit3("Color", col))
                l.color = glm::vec3(col[0], col[1], col[2]);

           
            if (s.enableShadows) {
                ImGui::Checkbox("Cast shadow", &l.castShadow);
            } else {
                ImGui::TextDisabled("Shadows disabled globally");
            }


            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.7f, 0.1f, 0.1f, 1.0f));
            if (ImGui::Button("Delete")) deleteIdx = i;
            ImGui::PopStyleColor();

            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (deleteIdx >= 0) s.lights.erase(s.lights.begin() + deleteIdx);
    ImGui::End();
}


#include "models.h"

inline void UI_DrawMaterialEditor(UIState& s) {
    ImGui::Begin("Material Editor");

    if (!s.sceneRef || s.sceneRef->empty()) {
        ImGui::Text("No models loaded.");
        ImGui::End();
        return;
    }

    std::vector<model>& scene = *s.sceneRef;

    for (int mi = 0; mi < (int)scene.size(); mi++) 
    {
        ImGui::PushID(mi);
        model& mdl = scene[mi];

        bool modelOpen = ImGui::TreeNode("", "%d: %s", mi, mdl.name.c_str());
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        ImGui::Checkbox("Vis", &mdl.visible);
        int i=mi;
        if (modelOpen) {
            // float sx = glm::length(glm::vec3(scene[i].models[0]));
            // float sy = glm::length(glm::vec3(scene[i].models[1]));
            // float sz = glm::length(glm::vec3(scene[i].models[2]));
            // float maxScale = std::max({sx, sy, sz});
            // float worldRadius = scene[i].radius * maxScale;
            // glm::vec3 wc = glm::vec3(scene[i].models * glm::vec4(scene[i].centre, 1.0f));

            // if (ImGui::TreeNode("", "%d: %s", i, scene[i].name.c_str())) {
            //     ImGui::Text("Local centre:  (%.2f, %.2f, %.2f)",
            //                 scene[i].centre.x, scene[i].centre.y, scene[i].centre.z);
            //     ImGui::Text("World centre:  (%.2f, %.2f, %.2f)", wc.x, wc.y, wc.z);
            //     ImGui::Text("Local radius:  %.3f", scene[i].radius);
            //     ImGui::Text("World radius:  %.3f", worldRadius);
            //     ImGui::Text("Scale:         (%.2f, %.2f, %.2f)", sx, sy, sz);

            //     // Show light-space Z for first shadow-casting light, cascade 0
            //     if (s.enableShadows) {
            //         for (auto& l : s.lights) {
            //             if (!l.enabled || !l.castShadow || !l.shadowData.initialized) continue;

            //             glm::mat4 vtlc = l.shadowData.cascades[0].viewToLightClip;
            //             glm::vec3 viewCentre = glm::vec3(s.view * glm::vec4(wc, 1.0f));
            //             glm::vec4 lightClip = vtlc * glm::vec4(viewCentre, 1.0f);
            //             glm::vec3 ndc = glm::vec3(lightClip) / lightClip.w;
            //             glm::vec4 lightCentre = s.debugview* glm::vec4(viewCentre, 1.0f);
            //             ImGui::Separator();
            //             ImGui::TextColored(ImVec4(1,1,0,1), "Shadow (cascade 0):");
            //             ImGui::Text("Light view:    (%.2f, %.2f, %.2f, %.2f)",
            //                         lightCentre.x, lightCentre.y, lightCentre.z, lightCentre.w);

            //             bool inFrustum = (ndc.x >= -1 && ndc.x <= 1 &&
            //                             ndc.y >= -1 && ndc.y <= 1 &&
            //                             ndc.z >= -1 && ndc.z <= 1);
            //             if (inFrustum)
            //                 ImGui::TextColored(ImVec4(0,1,0,1), "IN cascade 0");
            //             else
            //                 ImGui::TextColored(ImVec4(1,0,0,1), "OUTSIDE cascade 0");

            //             break;  // first shadow light only
            //         }
            //     }

            //     ImGui::TreePop();
            // }

            ImGui::Text("Meshes: %d", (int)mdl.meshes.size());

            auto matIndices = mdl.getUniqueMaterialIndices();

            for (int gi = 0; gi < (int)matIndices.size(); gi++) {
                unsigned int matIdx = matIndices[gi];
                auto meshIds = mdl.getMeshesForMaterial(matIdx);
                if (meshIds.empty()) continue;

                Mesh& rep = mdl.meshes[meshIds[0]];
                MaterialData& mat = rep.material;

                ImGui::PushID(gi);

                bool matOpen = ImGui::TreeNode("",
                    "Material: \"%s\" (%d meshes)", mat.name.c_str(), (int)meshIds.size());

                if (matOpen) {
                    ImGui::TextDisabled("Meshes:");
                    ImGui::SameLine();
                    for (int k = 0; k < (int)meshIds.size(); k++) {
                        if (k > 0) ImGui::SameLine();
                        ImGui::TextDisabled("%d", meshIds[k]);
                    }

                    if (ImGui::RadioButton("Blinn-Phong", !rep.iscooktorrence))
                        rep.iscooktorrence = false;
                    ImGui::SameLine();
                    if (ImGui::RadioButton("Cook-Torrance", rep.iscooktorrence))
                        rep.iscooktorrence = true;
                    ImGui::Separator();

                    float kd[3] = {mat.Kd.x, mat.Kd.y, mat.Kd.z};
                    if (ImGui::ColorEdit3("Kd (Diffuse)", kd))
                        mat.Kd = glm::vec3(kd[0], kd[1], kd[2]);

                    float ka[3] = {mat.Ka.x, mat.Ka.y, mat.Ka.z};
                    if (ImGui::ColorEdit3("Ka (Ambient)", ka))
                        mat.Ka = glm::vec3(ka[0], ka[1], ka[2]);

                    float ks[3] = {mat.Ks.x, mat.Ks.y, mat.Ks.z};
                    if (ImGui::ColorEdit3("Ks (Specular)", ks))
                        mat.Ks = glm::vec3(ks[0], ks[1], ks[2]);

                    ImGui::Separator();

                    ImGui::SliderFloat("Shininess (Ns)", &mat.Ns, 0.0f, 1000.0f);
                    ImGui::SliderFloat("Refractive Index (Ni)", &mat.Ni, 1.0f, 3.0f);
                    ImGui::SliderFloat("Opacity (d)", &mat.d, 0.0f, 1.0f);
                    ImGui::SliderFloat("Roughness", &mat.rough, 0.01f, 1.0f);

                    ImGui::Separator();

                    ImGui::Text("Textures:");

                    if (mat.hasDiffuseMap)
                        ImGui::Checkbox("Diffuse map", &mat.useDiffuseMap);
                    else
                        ImGui::TextDisabled("Diffuse map: none");

                    if (mat.hasSpecularMap)
                        ImGui::Checkbox("Specular map", &mat.useSpecularMap);
                    else
                        ImGui::TextDisabled("Specular map: none");

                    if (mat.hasAmbientMap)
                        ImGui::Checkbox("Ambient map", &mat.useAmbientMap);
                    else
                        ImGui::TextDisabled("Ambient map: none");

                    if (mat.hasNormalMap)
                        ImGui::Checkbox("Normal map", &mat.useNormalMap);
                    else
                        ImGui::TextDisabled("Normal map: none");

                    if (mat.hasOpacityMap)
                        ImGui::Checkbox("Opacity map", &mat.useOpacityMap);
                    else
                        ImGui::TextDisabled("Opacity map: none");

                    if (mat.hasNormalMap)
                        ImGui::Checkbox("Flip normal Y (DirectX->OpenGL)", &mat.flipNormalY);

                    ImGui::Separator();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.7f, 1.0f));
                    if (ImGui::Button("Reset to original"))
                        mat.resetToOriginal();
                    ImGui::PopStyleColor();

                    for (int k = 1; k < (int)meshIds.size(); k++) {
                        Mesh& sib = mdl.meshes[meshIds[k]];
                        sib.material.Kd    = mat.Kd;
                        sib.material.Ks    = mat.Ks;
                        sib.material.Ka    = mat.Ka;
                        sib.material.Ns    = mat.Ns;
                        sib.material.Ni    = mat.Ni;
                        sib.material.d     = mat.d;
                        sib.material.rough = mat.rough;
                        sib.material.useDiffuseMap  = mat.useDiffuseMap;
                        sib.material.useSpecularMap = mat.useSpecularMap;
                        sib.material.useAmbientMap  = mat.useAmbientMap;
                        sib.material.useNormalMap   = mat.useNormalMap;
                        sib.material.useOpacityMap  = mat.useOpacityMap;
                        sib.material.flipNormalY    = mat.flipNormalY;
                        sib.iscooktorrence = rep.iscooktorrence;
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    ImGui::End();
}



inline void UI_DrawPanels(UIState& s) {
    UI_DrawScenePanel(s);
    UI_DrawLightPanel(s);
    UI_DrawMaterialEditor(s);
}

inline void UI_Render(GLFWwindow* window) {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}


inline bool UI_WantsKeyboard() { return ImGui::GetIO().WantCaptureKeyboard; }
inline bool UI_WantsMouse()    { return ImGui::GetIO().WantCaptureMouse; }


inline void UI_CleanupState(UIState& s) {
    glDeleteFramebuffers(1, &s.pickFBO);
    glDeleteTextures(1, &s.pickColor);
    glDeleteRenderbuffers(1, &s.pickDepth);
    glDeleteVertexArrays(1, &s.crossVAO);
    glDeleteBuffers(1, &s.crossVBO);
}