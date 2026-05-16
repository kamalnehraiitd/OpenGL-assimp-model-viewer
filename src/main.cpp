#include "ui.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include "shaders.h"
#include "models.h"
#include "shadow.h"
#include "ao.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
namespace fs = std::filesystem;
#include "oit.h"

float oitThreshold = 0.99f;
UIState S;

void screenshotCurrentFBO(const std::string& filename, int width, int height) {
    std::vector<unsigned char> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::vector<unsigned char> flipped(width * height * 3);
    for (int y = 0; y < height; y++)
        memcpy(&flipped[y * width * 3], &pixels[(height - 1 - y) * width * 3], width * 3);
    stbi_write_png(filename.c_str(), width, height, 3, flipped.data(), width * 3);
    std::cout << "Screenshot saved: " << filename << " (" << width << "x" << height << ")\n";
}

struct GridData { GLuint VAO = 0, VBO = 0; int vertexCount = 0; };
struct AxisData { GLuint VAO = 0, VBO = 0; };

GridData createGrid(float size, float step) {
    std::vector<float> verts;
    for (float x = -size; x <= size; x += step) {
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(-size);
        verts.push_back(x); verts.push_back(0.0f); verts.push_back(size);
    }
    for (float z = -size; z <= size; z += step) {
        verts.push_back(-size); verts.push_back(0.0f); verts.push_back(z);
        verts.push_back(size);  verts.push_back(0.0f); verts.push_back(z);
    }
    GridData grid;
    grid.vertexCount = verts.size() / 3;
    glGenVertexArrays(1, &grid.VAO);
    glGenBuffers(1, &grid.VBO);
    glBindVertexArray(grid.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, grid.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);
    return grid;
}

AxisData createAxes(float length) {
    float verts[] = {
        0.0f, -0.1f, 0.0f, 1.0f, 0.0f, 0.0f, length, -0.1f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, -0.1f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, length, 0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, -0.1f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, length, 0.0f, 0.0f, 1.0f,
    };
    AxisData axes;
    glGenVertexArrays(1, &axes.VAO);
    glGenBuffers(1, &axes.VBO);
    glBindVertexArray(axes.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, axes.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);
    return axes;
}

void renderGrid(myshader& gridShader, GridData& grid) {
    gridShader.use();
    gridShader.unifomrMat4("view", S.view);
    gridShader.unifomrMat4("proj", S.projection);
    gridShader.uniformVec3("gridColor", glm::vec3(0.3f));
    glBindVertexArray(grid.VAO);
    glDrawArrays(GL_LINES, 0, grid.vertexCount);
    glBindVertexArray(0);
}

void renderAxes(myshader& axisShader, AxisData& axes) {
    axisShader.use();
    axisShader.unifomrMat4("view", S.view);
    axisShader.unifomrMat4("proj", S.projection);
    glLineWidth(3.0f);
    glBindVertexArray(axes.VAO);
    glDrawArrays(GL_LINES, 0, 6);
    glBindVertexArray(0);
    glLineWidth(1.0f);
}

void initMSAAFBO(UIState& s) {
    if (s.msaaFBO) { glDeleteFramebuffers(1, &s.msaaFBO); glDeleteTextures(1, &s.msaaColor); glDeleteRenderbuffers(1, &s.msaaDepth); }
    int samples = s.aaSamplesX * s.aaSamplesY;
    int maxSamples; glGetIntegerv(GL_MAX_SAMPLES, &maxSamples);
    if (samples > maxSamples) samples = maxSamples;
    int fbW, fbH; glfwGetFramebufferSize(glfwGetCurrentContext(), &fbW, &fbH);
    glGenFramebuffers(1, &s.msaaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.msaaFBO);
    glGenTextures(1, &s.msaaColor);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, s.msaaColor);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, fbW, fbH, GL_TRUE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, s.msaaColor, 0);
    glGenRenderbuffers(1, &s.msaaDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, s.msaaDepth);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, samples, GL_DEPTH24_STENCIL8, fbW, fbH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s.msaaDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cerr << "MSAA FBO not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s.aaNeedsRestart = false;
}

void initSSAAFBO(UIState& s) {
    if (s.msaaFBO) { glDeleteFramebuffers(1, &s.msaaFBO); glDeleteTextures(1, &s.msaaColor); glDeleteRenderbuffers(1, &s.msaaDepth); }
    int fbW, fbH; glfwGetFramebufferSize(glfwGetCurrentContext(), &fbW, &fbH);
    int ssW = fbW * s.aaSamplesX, ssH = fbH * s.aaSamplesY;
    glGenFramebuffers(1, &s.msaaFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, s.msaaFBO);
    glGenTextures(1, &s.msaaColor);
    glBindTexture(GL_TEXTURE_2D, s.msaaColor);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ssW, ssH, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s.msaaColor, 0);
    glGenRenderbuffers(1, &s.msaaDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, s.msaaDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, ssW, ssH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, s.msaaDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cerr << "SSAA FBO not complete!\n";
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    s.aaNeedsRestart = false;
}

void rebuildAAFBO(UIState& s) { if (s.aaMethod == 0) initMSAAFBO(s); else initSSAAFBO(s); }

void initCamera(std::vector<model>& scene) {
    S.camera.yaw = -90.0f; S.camera.pitch = 0.0f; S.camera.distfactor = 1.0f;
    updateCameraVectors(S);
    S.camera.position = scene.size() ? scene[0].centre - S.camera.front : glm::vec3(0);
    S.view = glm::lookAt(S.camera.position, S.camera.position + S.camera.front, S.camera.up);
}

void loadLights() {
    std::ifstream file(S.dirpath + "lights");
    if (!file.is_open()) { std::cerr << "Failed to open light file\n"; return; }
    S.lights.clear();
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        int sp; float x, y, z, r, g, b;
        if (ss >> sp >> x >> y >> z >> r >> g >> b) {
            DirLightData light;
            light.direction = glm::vec3(x, y, z); light.color = glm::vec3(r, g, b);
            light.enabled = true; light.space = sp; light.castShadow = true;
            S.lights.push_back(light);
        }
    }
}

void loadScene(std::vector<model>& scene) {
    if (!fs::exists(S.dirpath) || !fs::is_directory(S.dirpath)) { std::cerr << "Error: directory not found.\n"; exit(EXIT_FAILURE); }
    for (const auto& entry : fs::directory_iterator(S.dirpath))
        if (entry.is_regular_file() && entry.path().extension() == ".obj")
            scene.emplace_back(entry.path().string(), S.dirpath);
    loadLights();
}

void processInput(GLFWwindow* window) {
    if (UI_WantsKeyboard()) return;
    float speed = S.deltatime * 4.0f;
    glm::vec3 offset(0);
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)    offset += S.camera.front * speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)    offset -= S.camera.front * speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)    offset -= S.camera.right * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)    offset += S.camera.right * speed;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)   offset += S.worldup * speed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) offset -= S.worldup * speed;
    S.camera.position += offset;
    if (S.selectedobj && glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
       { 
        S.selectedobj->models = glm::translate(glm::mat4(1.0f), offset) * S.selectedobj->models;

        }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (UI_WantsKeyboard()) return;
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        S.trackball = !S.trackball;
        if (S.trackball && S.hoveredIndex >= 0 && S.hoveredIndex < (int)S.sceneRef->size()) {
            model& tgt = (*S.sceneRef)[S.hoveredIndex];
            S.trackballTarget = glm::vec3(tgt.models * glm::vec4(tgt.centre, 1.0f));
            float sx = glm::length(glm::vec3(tgt.models[0]));
            float sy = glm::length(glm::vec3(tgt.models[1]));
            float sz = glm::length(glm::vec3(tgt.models[2]));
            float worldRadius = tgt.radius * std::max({sx, sy, sz});
            S.camera.distfactor=worldRadius;
        }
    }
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        S.cursorLocked = !S.cursorLocked;
        glfwSetInputMode(window, GLFW_CURSOR, S.cursorLocked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        if (S.cursorLocked) S.firstMouse = false;
    }
    if (key == GLFW_KEY_G && action == GLFW_PRESS) S.showGrid = !S.showGrid;
    if (key == GLFW_KEY_P && action == GLFW_PRESS) S.takeScreenshot = true;
}

void mouse_callback(GLFWwindow* window, double x, double y) {

    if (!S.cursorLocked || UI_WantsMouse()) return;
    if (!S.firstMouse) { S.lastx = x; S.lasty = y; S.firstMouse = true; }
    float dx = (x - S.lastx) * 0.1f, dy = (S.lasty - y) * 0.1f;
    S.lastx = x; S.lasty = y;

    if(S.isMouseDown)
    {
        
        
        float distance=abs(x-S.clickStartX)+abs(y-S.clickStartY);
        if(distance>S.DRAG_THRESHOLD){
            S.isDragging = true;
        }
        if (S.selectedobj && S.isDragging) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) {
            float rotSpeed = 5.0f;
            glm::vec3 wc = glm::vec3(S.selectedobj->models * glm::vec4(S.selectedobj->centre, 1.0f));
            glm::mat4 rot(1.0f);
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
                rot = glm::rotate(rot, glm::radians(dx * rotSpeed), S.camera.front);
            else {
                rot = glm::rotate(rot, glm::radians(dx * rotSpeed), S.camera.up);
                rot = glm::rotate(rot, glm::radians(dy * rotSpeed), S.camera.right);
            }
            glm::mat4 toO = glm::translate(glm::mat4(1.0f), -wc);
            glm::mat4 back = glm::translate(glm::mat4(1.0f), wc);
            S.selectedobj->models = back * rot * toO * S.selectedobj->models;
       
        } else {
            float sx = glm::length(glm::vec3(S.selectedobj->models[0]));
            float sy = glm::length(glm::vec3(S.selectedobj->models[1]));
            float sz = glm::length(glm::vec3(S.selectedobj->models[2]));
            float ms = (S.selectedobj->radius * std::max(sx, std::max(sy, sz))) * 0.1f;
            glm::vec3 move = S.camera.right * dx * ms + S.camera.up * dy * ms;
            S.selectedobj->models = glm::translate(glm::mat4(1.0f), move) * S.selectedobj->models;
      
            glm::vec3 nwc = glm::vec3(S.selectedobj->models * glm::vec4(S.selectedobj->centre, 1.0f));
            S.camera.front = glm::normalize(nwc - S.camera.position);
            S.camera.pitch = glm::degrees(asin(S.camera.front.y));
            S.camera.yaw = glm::degrees(atan2(S.camera.front.z, S.camera.front.x));
            S.camera.pitch = glm::clamp(S.camera.pitch, -89.0f, 89.0f);
            S.camera.right = glm::normalize(glm::cross(S.camera.front, S.worldup));
            S.camera.up = glm::normalize(glm::cross(S.camera.right, S.camera.front));
        }
        } 
}
else {
        if (S.trackball && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            S.camera.distfactor += dy * 0.2f;
            if (S.camera.distfactor < 0.1f) S.camera.distfactor = 0.1f;
        } else {
            S.camera.yaw += dx; S.camera.pitch += dy;
        }
        S.camera.pitch = glm::clamp(S.camera.pitch, -89.0f, 89.0f);
        updateCameraVectors(S);
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    if (UI_WantsMouse() || !S.selectedobj||S.trackball) return;
    float sf = 1.0f + (float)yoffset * 0.1f;
    if (sf < 0.1f) sf = 0.1f;
    glm::vec3 wc = glm::vec3(S.selectedobj->models * glm::vec4(S.selectedobj->centre, 1.0f));
    glm::mat4 t = glm::translate(glm::mat4(1.0f), wc);
    glm::mat4 s = glm::scale(glm::mat4(1.0f), glm::vec3(sf));
    glm::mat4 ti = glm::translate(glm::mat4(1.0f), -wc);
    S.selectedobj->models = t * s * ti * S.selectedobj->models;

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    int sw, sh; glfwGetWindowSize(window, &sw, &sh);
    S.scrWidth = sw; S.scrHeight = sh;
    resizePickFBO(S, width, height);
    if (S.enableAA) rebuildAAFBO(S);
    resizeAO(S, width, height);
    resizeOIT(S, width, height);
}

int pickObject(GLFWwindow* window, myshader& pickShader, std::vector<model>& scene) {
    int fbW, fbH; glfwGetFramebufferSize(window, &fbW, &fbH);
    glBindFramebuffer(GL_FRAMEBUFFER, S.pickFBO);
    glViewport(0, 0, fbW, fbH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    pickShader.use();
    pickShader.unifomrMat4("view", S.view);
    pickShader.unifomrMat4("proj", S.projection);
    for (int i = 0; i < (int)scene.size(); i++) {
        if (!scene[i].visible) continue;
        int id = i + 1;
        pickShader.uniformVec4("color", glm::vec4((id & 0xFF) / 255.0f, ((id >> 8) & 0xFF) / 255.0f, ((id >> 16) & 0xFF) / 255.0f, 1.0f));
        pickShader.unifomrMat4("model", scene[i].models);
        for (int j = 0; j < (int)scene[i].meshes.size(); j++) {
            glBindVertexArray(scene[i].meshes[j].VAO);
            glDrawElements(GL_TRIANGLES, scene[i].meshes[j].indexcount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }
    unsigned char pixel[3];
    glReadPixels(fbW / 2, fbH / 2, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbW, fbH);
    return (pixel[0] + (pixel[1] << 8) + (pixel[2] << 16)) - 1;
}

void uploadLights(myshader& shader) {
    int camCount = 0, worldCount = 0;
    for (auto& l : S.lights) {
        if (!l.enabled) continue;
        if (l.space == 0) {
            std::string base = "camera_lights[" + std::to_string(camCount) + "]";
            float len = glm::length(l.direction);
            glm::vec3 dir = (len > 0.001f) ? l.direction / len : l.direction;
            shader.uniformVec3((base + ".direction").c_str(), dir);
            shader.uniformVec3((base + ".color").c_str(), l.color);
            camCount++;
        } else {
            std::string base = "world_lights[" + std::to_string(worldCount) + "]";
            shader.uniformVec3((base + ".direction").c_str(), glm::mat3(S.view) * l.direction);
            shader.uniformVec3((base + ".color").c_str(), l.color);
            worldCount++;
        }
    }
    shader.uniformi("numLights_camera", camCount);
    shader.uniformi("numLights_world", worldCount);
}

void handleSelection(GLFWwindow* window, myshader& pickShader, std::vector<model>& scene) {
    int picked = pickObject(window, pickShader, scene);
    S.hoveredIndex = picked;
    S.selected = (picked >= 0 && picked < (int)scene.size()) ? scene[picked].name : "none";
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (UI_WantsMouse()||!S.cursorLocked) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            S.isMouseDown = true;
            S.isDragging = false;
            glfwGetCursorPos(window, &S.clickStartX, &S.clickStartY);
        } else if (action == GLFW_RELEASE) {
            S.isMouseDown = false;
            if (!S.isDragging) {
                S.selectedindex = S.hoveredIndex;
                if (S.selectedindex >= 0 && S.selectedindex < (int)S.sceneRef->size()) {
                    S.selectedobj = &(*S.sceneRef)[S.selectedindex];
                } else {
                    S.selectedobj = nullptr;
                    S.selectedindex = -1;
                }
            }
            S.isDragging = false;
        }
    }
}

void clearHoverHighlight(std::vector<model>& scene) {
    if (S.hoveredIndex >= 0 && S.hoveredIndex < (int)scene.size() && scene[S.hoveredIndex].selection == 1)
        scene[S.hoveredIndex].selection = 0;
}

void renderCrosshair(myshader& crossShader) {
    crossShader.use();
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(S.crossVAO);
    glDrawArrays(GL_LINES, 0, 4);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

int main(int argc, char* argv[]) {
    S.dirpath = ".";
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-aa" && i + 1 < argc) {
            std::string val = argv[++i]; int mx = 0, my = 0;
            if (sscanf(val.c_str(), "%dx%d", &mx, &my) == 2 && mx > 0 && my > 0) {
                S.enableAA = true; S.aaSamplesX = mx; S.aaSamplesY = my;
            }
        } else if (arg == "-shadow") { S.enableShadows = true; S.PCF = true;
        } else if (arg == "-ao" && i + 1 < argc) { float f = std::stof(argv[++i]); S.enableAO = true; S.aoFactor = f; S.scaleEntireScene = true;
        } else { S.dirpath = arg; }
    }
    if (S.dirpath.back() != '/' && S.dirpath.back() != '\\') S.dirpath += "/";

    glfwInit();
    const char* glsl_version = "#version 410";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(S.scrWidth, S.scrHeight, "Renderer", NULL, NULL);
    if (!window) { std::cout << "error in window\n"; return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    S.window = window;
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cout << "error in glad\n"; return -1; }

    UI_Init(window, glsl_version);
    initPickFBO(S, window);
    initCrosshair(S);
    rebuildAAFBO(S);

    myshader b("shaders/bling.vs", "shaders/bling.fs");
    myshader cross("shaders/cross.vs", "shaders/cross.fs");
    myshader pick("shaders/pick.vs", "shaders/pick.fs");
    myshader gridShader("shaders/grid.vs", "shaders/grid.fs");
    myshader axisShader("shaders/axis.vs", "shaders/axis.fs");
    myshader shadow("shaders/shadow.vs", "shaders/shadow.fs");
    myshader outline("shaders/outline.vs", "shaders/outline.fs");

    initAO(S);
    initOIT(S);

    std::vector<model> scene;
    loadScene(scene);
    S.sceneObjectCount = (int)scene.size();
    S.sceneRef = &scene;

    GridData grid = createGrid(50.0f, 2.0f);
    AxisData axes = createAxes(50.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    S.projection = glm::perspective(45.0f, (float)S.scrWidth / (float)S.scrHeight, 0.1f, 100.0f);
    initCamera(scene);
    float lastframetime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        if (S.aaNeedsRestart && S.enableAA) rebuildAAFBO(S);

        float now = glfwGetTime();
        S.deltatime = now - lastframetime;
        lastframetime = now;

        glfwPollEvents();
        processInput(window);

        UI_NewFrame();
        UI_SyncState(S);
        UI_DrawPanels(S);

        if (S.trackball && S.hoveredIndex >= 0 && S.hoveredIndex < (int)scene.size()) {
            S.camera.position = glm::vec3(scene[S.hoveredIndex].models * glm::vec4(scene[S.hoveredIndex].centre, 1.0f)) - (S.camera.front * S.camera.distfactor);
        } else { S.camera.distfactor = 1.0f; }
        S.view = glm::lookAt(S.camera.position, S.camera.position + S.camera.front, S.camera.up);

        handleSelection(window, pick, scene);

        generateAllShadowMaps(S, shadow);
        if (S.enableAO) executeAO(S, scene);

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);

        if (S.enableAA) {
            glBindFramebuffer(GL_FRAMEBUFFER, S.msaaFBO);
            if (S.aaMethod == 1) glViewport(0, 0, fbW * S.aaSamplesX, fbH * S.aaSamplesY);
            else glViewport(0, 0, fbW, fbH);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbW, fbH);
        }

        glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        b.use();
        b.unifomrMat4("view", S.view);
        b.unifomrMat4("proj", S.projection);
        b.uniformVec3("viewPos", S.camera.position);
        uploadLights(b);
        b.uniformf("cookambientStrength", S.cookambientStrength);
        b.uniformf("blinambientStrength", S.blinambientStrength);
        uploadAllShadowUniforms(S, b);
        bindAOTexture(S, b);
        shadow.uniformi("debugcascades", S.shadowDebugVis);
        b.uniformVec2("framebufferSize", glm::vec2((float)fbW, (float)fbH));
        b.uniformi("scaleEntireScene", S.scaleEntireScene ? 1 : 0);
        b.uniformi("oitPass", 0);
        for (int i = 0; i < (int)scene.size(); i++) {
            if (!scene[i].visible) continue;

            if (S.selectedobj == &scene[i]) {
                glStencilFunc(GL_ALWAYS, 1, 0xFF);
                glStencilMask(0xFF);
            } else {
                glStencilFunc(GL_ALWAYS, 0, 0xFF);
                glStencilMask(0x00);
            }

            for (int j = 0; j < (int)scene[i].meshes.size(); j++) {
                if (scene[i].meshes[j].material.d < oitThreshold) continue;
                scene[i].meshes[j].drawmesh(b, scene[i].models, scene[i].selection);
            }
        }
        glStencilMask(0x00);

        // if (S.selectedobj&&!S.trackball) {
        //     glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        //     glStencilMask(0x00);
        //     glDisable(GL_DEPTH_TEST);

        //     outline.use();
        //     outline.unifomrMat4("view", S.view);
        //     outline.unifomrMat4("proj", S.projection);
        //     outline.unifomrMat4("model", S.selectedobj->models);
        //     outline.uniformVec3("outlineColor", glm::vec3(1.0f, 0.6f, 0.0f));

        //     for (int j = 0; j < (int)S.selectedobj->meshes.size(); j++) {
        //         if (S.selectedobj->meshes[j].material.d < oitThreshold) continue;
        //         glBindVertexArray(S.selectedobj->meshes[j].VAO);
        //         glDrawElements(GL_TRIANGLES, S.selectedobj->meshes[j].indexcount, GL_UNSIGNED_INT, 0);
        //         glBindVertexArray(0);
        //     }

        //     glEnable(GL_DEPTH_TEST);
        //     glStencilFunc(GL_ALWAYS, 0, 0xFF);
        //     glStencilMask(0xFF);
        // }

        if (S.showGrid) { renderGrid(gridShader, grid); renderAxes(axisShader, axes); }
        clearHoverHighlight(scene);
        renderCrosshair(cross);

 
        bool hasTransparent = false;
        for (int i = 0; i < (int)scene.size() && !hasTransparent; i++)
            if (scene[i].visible)
                for (int j = 0; j < (int)scene[i].meshes.size(); j++)
                    if (scene[i].meshes[j].material.d < oitThreshold) { hasTransparent = true; break; }

        if (hasTransparent) {
            glStencilMask(0x00);

            if (S.enableOIT) {
                if (S.oitMethod == 0) {
                    weightedOIT_begin(S, fbW, fbH);
                    b.use();
                    b.uniformi("oitPass", 1);
                    for (int i = 0; i < (int)scene.size(); i++) {
                        if (!scene[i].visible) continue;
                        for (int j = 0; j < (int)scene[i].meshes.size(); j++) {
                            if (scene[i].meshes[j].material.d >= oitThreshold) continue;
                            scene[i].meshes[j].drawmesh(b, scene[i].models, scene[i].selection);
                        }
                    }
                    weightedOIT_end(S, fbW, fbH);
                }
            } else {
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);
                b.use();
                b.uniformi("oitPass", 0);
                for (int i = 0; i < (int)scene.size(); i++) {
                    if (!scene[i].visible) continue;
                    for (int j = 0; j < (int)scene[i].meshes.size(); j++) {
                        if (scene[i].meshes[j].material.d >= oitThreshold) continue;
                        scene[i].meshes[j].drawmesh(b, scene[i].models, scene[i].selection);
                    }
                }
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);
            }
        }

        if (S.enableAA) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, S.msaaFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            if (S.aaMethod == 1) glBlitFramebuffer(0, 0, fbW * S.aaSamplesX, fbH * S.aaSamplesY, 0, 0, fbW, fbH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            else glBlitFramebuffer(0, 0, fbW, fbH, 0, 0, fbW, fbH, GL_COLOR_BUFFER_BIT, GL_LINEAR);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbW, fbH);
        }

        if (S.takeScreenshot) {
            auto now = std::chrono::system_clock::now();
            auto in_time_t = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&in_time_t), "%Y%m%d_%H%M%S");
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            screenshotCurrentFBO("screenshot_" + ss.str() + ".png", fbW, fbH);
            S.takeScreenshot = false;
        }

        UI_Render(window);
        glfwSwapBuffers(window);
    }

    UI_Shutdown();
    UI_CleanupState(S);
    if (S.msaaFBO) { glDeleteFramebuffers(1, &S.msaaFBO); glDeleteTextures(1, &S.msaaColor); glDeleteRenderbuffers(1, &S.msaaDepth); }
    glDeleteVertexArrays(1, &grid.VAO); glDeleteBuffers(1, &grid.VBO);
    glDeleteVertexArrays(1, &axes.VAO); glDeleteBuffers(1, &axes.VBO);
    cleanupAO(S);
    cleanupOIT(S);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}