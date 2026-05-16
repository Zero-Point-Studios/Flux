#include "3DRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include "utils/PathHelper.h"

namespace Flux
{
    static std::string LoadShaderFromFile(const std::string& filepath)
    {
        size_t size;
        char* shaderCode = (char*)SDL_LoadFile(filepath.c_str(), &size);

        if (!shaderCode) {
            std::cerr << "Failed to load shader file: " << filepath << " - " << SDL_GetError() << std::endl;
            Output::addLog("SHADER ERROR: Could not find " + filepath);
            return "";
        }

        std::string result(shaderCode, size);

        SDL_free(shaderCode);
        return result;
    }

    static unsigned int compileShader(GLenum type, const char* src)
    {
        unsigned int s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        int ok;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, sizeof(log), nullptr, log);
            std::cerr << "Shader compile error:\n" << log << "\n";
        }
        return s;
    }

    static unsigned int linkProgram(const char* vs, const char* fs)
    {
        unsigned int v = compileShader(GL_VERTEX_SHADER,   vs);
        unsigned int f = compileShader(GL_FRAGMENT_SHADER, fs);
        unsigned int p = glCreateProgram();
        glAttachShader(p, v);
        glAttachShader(p, f);
        glLinkProgram(p);
        int ok;
        glGetProgramiv(p, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(p, sizeof(log), nullptr, log);
            std::cerr << "Program link error:\n" << log << "\n";
        }
        glDeleteShader(v);
        glDeleteShader(f);
        return p;
    }

    static void set1i(unsigned int p, const char* n, int v)         { glUniform1i(glGetUniformLocation(p, n), v); }
    static void set1f(unsigned int p, const char* n, float v)       { glUniform1f(glGetUniformLocation(p, n), v); }
    static void set3f(unsigned int p, const char* n, glm::vec3 v)   { glUniform3fv(glGetUniformLocation(p, n), 1, glm::value_ptr(v)); }
    static void setMat3(unsigned int p, const char* n, glm::mat3 m) { glUniformMatrix3fv(glGetUniformLocation(p, n), 1, GL_FALSE, glm::value_ptr(m)); }
    static void setMat4(unsigned int p, const char* n, glm::mat4 m) { glUniformMatrix4fv(glGetUniformLocation(p, n), 1, GL_FALSE, glm::value_ptr(m)); }

    void Renderer3D::Init()
    {
        std::string vCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/vertSrc.glsl"));
        std::string fCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/fragSrc.glsl"));
        shaderProgram     = linkProgram(vCode.c_str(), fCode.c_str());
        InitBillboard();
    }

    void Renderer3D::InitShadowMap(int resolution)
    {
        shadowResolution = resolution;

        glGenFramebuffers(1, &shadowFBO);
        glGenTextures(1, &shadowDepthTex);
        glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F,
                     resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float border[] = { 1.f, 1.f, 1.f, 1.f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTex, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cerr << "Shadow FBO incomplete!\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        const char* depthVS = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 lightSpaceMatrix;
uniform mat4 model;
void main() { gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0); }
)";
        const char* depthFS = R"(
#version 330 core
void main() {}
)";
        depthProgram = linkProgram(depthVS, depthFS);
        shadowReady  = true;
    }

    void Renderer3D::DrawDepthPass(const std::vector<SceneNode>& nodes, glm::vec3 lightDir)
    {
        if (!shadowReady) return;

        glm::vec3 lp  = -glm::normalize(lightDir) * 40.f;
        glm::mat4 lv  = glm::lookAt(lp, glm::vec3(0.f), glm::vec3(0.f, 1.f, 0.f));
        glm::mat4 lpr = glm::ortho(-30.f, 30.f, -30.f, 30.f, 0.5f, 120.f);
        lightSpaceMatrix = lpr * lv;

        glViewport(0, 0, shadowResolution, shadowResolution);
        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthProgram);
        setMat4(depthProgram, "lightSpaceMatrix", lightSpaceMatrix);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glEnable(GL_POLYGON_OFFSET_FILL);
        glPolygonOffset(2.0f, 4.0f);

        for (const auto& node : nodes) {
            if (node.type != NodeType::Mesh || !node.model) continue;
            setMat4(depthProgram, "model", node.GetTransformMatrix());
            for (auto& mesh : node.model->meshes) {
                glBindVertexArray(mesh.VAO);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
                glBindVertexArray(0);
            }
        }

        glDisable(GL_POLYGON_OFFSET_FILL);
        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Renderer3D::DrawScene(Model& model, unsigned int overrideTexID,
                               glm::mat4 modelMatrix, glm::mat4 view, glm::mat4 proj,
                               glm::vec3 cameraPos,
                               const std::vector<SceneNode>& lights,
                               float alpha, float roughness, float metallic,
                               float timeOfDay, glm::vec3 baseColor)
    {
        glUseProgram(shaderProgram);

        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        if (alpha < 1.0f) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
        }

        setMat4(shaderProgram, "model",       modelMatrix);
        setMat4(shaderProgram, "view",        view);
        setMat4(shaderProgram, "projection",  proj);
        set3f (shaderProgram, "viewPos",     cameraPos);
        set1f (shaderProgram, "alpha",       alpha);
        set1f (shaderProgram, "roughness",   roughness);
        set1f (shaderProgram, "metallic",    metallic);
        set1f (shaderProgram, "timeOfDay",   timeOfDay);
        set1i(shaderProgram, "isSelected", isSelected ? 1 : 0);

        glm::mat3 nm = glm::mat3(glm::transpose(glm::inverse(modelMatrix)));
        setMat3(shaderProgram, "normalMatrix", nm);

        if (shadowReady) {
            setMat4(shaderProgram, "lightSpaceMatrix", lightSpaceMatrix);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowDepthTex);
            set1i(shaderProgram, "shadowMap",    1);
            set1i(shaderProgram, "hasShadowMap", 1);
        } else {
            set1i(shaderProgram, "hasShadowMap", 0);
        }

        bool      hasSunLight  = false;
        bool      hasMoonLight  = false;
        int       numPoint      = 0, numSpot = 0;
        bool      hasSurface    = false;
        glm::vec3 surfCol(1.f);
        float     surfInt = 0.f;

        for (const auto& node : lights) {
            switch (node.type) {
            case NodeType::DirectionalLight:
                if (node.isLightingNode) {
                    float tod = node.light.timeOfDay;
                    const float kDawn = 6.f, kDusk = 18.f, kBlend = 1.f;
                    float nightT = 0.f;
                    if (tod >= kDusk + kBlend || tod <= kDawn - kBlend) {
                        nightT = 1.f;
                    } else if (tod > kDusk - kBlend && tod < kDusk + kBlend) {
                        nightT = (tod - (kDusk - kBlend)) / (2.f * kBlend);
                    } else if (tod > kDawn - kBlend && tod < kDawn + kBlend) {
                        nightT = 1.f - (tod - (kDawn - kBlend)) / (2.f * kBlend);
                    } else {
                        nightT = 0.f;
                    }

                    float sunWeight  = 1.f - nightT;
                    float moonWeight = nightT;

                    if (sunWeight > 0.f && !hasSunLight) {
                        hasSunLight = true;
                        set3f(shaderProgram, "sunLightDir",       node.light.direction);
                        set3f(shaderProgram, "sunLightColor",     node.light.color);
                        set1f(shaderProgram, "sunLightIntensity", node.light.intensity * sunWeight);
                    }
                    if (moonWeight > 0.f && !hasMoonLight) {
                        hasMoonLight = true;
                        glm::vec3 moonDir = -node.light.direction;
                        set3f(shaderProgram, "moonLightDir",       moonDir);
                        set3f(shaderProgram, "moonLightColor",     node.light.moonColor);
                        set1f(shaderProgram, "moonLightIntensity", node.light.moonIntensity * moonWeight);
                    }
                } else {
                    if (!hasSunLight) {
                        hasSunLight = true;
                        set3f(shaderProgram, "sunLightDir",       node.light.direction);
                        set3f(shaderProgram, "sunLightColor",     node.light.color);
                        set1f(shaderProgram, "sunLightIntensity", node.light.intensity);
                    }
                }
                break;
            case NodeType::PointLight:
                if (numPoint < 8) {
                    auto idx = std::to_string(numPoint);
                    set3f(shaderProgram, ("pointPos["       + idx + "]").c_str(), node.position);
                    set3f(shaderProgram, ("pointColor["     + idx + "]").c_str(), node.light.color);
                    set1f(shaderProgram, ("pointIntensity[" + idx + "]").c_str(), node.light.intensity);
                    set1f(shaderProgram, ("pointRange["     + idx + "]").c_str(), node.light.range);
                    numPoint++;
                }
                break;
            case NodeType::SpotLight:
                if (numSpot < 4) {
                    auto idx = std::to_string(numSpot);
                    set3f(shaderProgram, ("spotPos["       + idx + "]").c_str(), node.position);
                    set3f(shaderProgram, ("spotDir["       + idx + "]").c_str(), node.light.direction);
                    set3f(shaderProgram, ("spotColor["     + idx + "]").c_str(), node.light.color);
                    set1f(shaderProgram, ("spotIntensity[" + idx + "]").c_str(), node.light.intensity);
                    set1f(shaderProgram, ("spotRange["     + idx + "]").c_str(), node.light.range);
                    set1f(shaderProgram, ("spotInner["     + idx + "]").c_str(), node.light.innerCutoff);
                    set1f(shaderProgram, ("spotOuter["     + idx + "]").c_str(), node.light.outerCutoff);
                    numSpot++;
                }
                break;
            case NodeType::SurfaceLight:
                if (!hasSurface) {
                    hasSurface = true;
                    surfCol    = node.light.color;
                    surfInt    = node.light.intensity;
                }
                break;
            default: break;
            }
        }

        set1i(shaderProgram, "hasSunLight",      hasSunLight  ? 1 : 0);
        set1i(shaderProgram, "hasMoonLight",     hasMoonLight ? 1 : 0);
        set1i(shaderProgram, "numPointLights",   numPoint);
        set1i(shaderProgram, "numSpotLights",    numSpot);
        set1i(shaderProgram, "hasSurface",       hasSurface ? 1 : 0);
        set3f(shaderProgram, "surfaceColor",     surfCol);
        set1f(shaderProgram, "surfaceIntensity", surfInt);

        for (auto& mesh : model.meshes) {
            bool useTex = (overrideTexID != 0);
            set1i(shaderProgram, "hasTexture", useTex ? 1 : 0);
            if (useTex) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, overrideTexID);
                set1i(shaderProgram, "albedoMap", 0);
            } else {
                glm::vec3 col;
                if (baseColor.r >= 0.f)          col = baseColor;
                else if (mesh.hasMtlColor)        col = mesh.matColor;
                else                              col = glm::vec3(0.8f);
                set3f(shaderProgram, "matColor", col);
            }

            glBindVertexArray(mesh.VAO);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, nullptr);
            glBindVertexArray(0);
        }

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    void Renderer3D::InitSkybox()
    {
        std::string vCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/skyboxVertShader.glsl"));
        std::string fCode = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/skyboxFragShader.glsl"));
        skyboxProgram = linkProgram(vCode.c_str(), fCode.c_str());

        float skyboxVerts[] = {
            -1.f,  1.f, -1.f,  -1.f, -1.f, -1.f,   1.f, -1.f, -1.f,
             1.f, -1.f, -1.f,   1.f,  1.f, -1.f,  -1.f,  1.f, -1.f,
            -1.f, -1.f,  1.f,  -1.f, -1.f, -1.f,  -1.f,  1.f, -1.f,
            -1.f,  1.f, -1.f,  -1.f,  1.f,  1.f,  -1.f, -1.f,  1.f,
             1.f, -1.f, -1.f,   1.f, -1.f,  1.f,   1.f,  1.f,  1.f,
             1.f,  1.f,  1.f,   1.f,  1.f, -1.f,   1.f, -1.f, -1.f,
            -1.f, -1.f,  1.f,  -1.f,  1.f,  1.f,   1.f,  1.f,  1.f,
             1.f,  1.f,  1.f,   1.f, -1.f,  1.f,  -1.f, -1.f,  1.f,
            -1.f,  1.f, -1.f,   1.f,  1.f, -1.f,   1.f,  1.f,  1.f,
             1.f,  1.f,  1.f,  -1.f,  1.f,  1.f,  -1.f,  1.f, -1.f,
            -1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,   1.f, -1.f, -1.f,
             1.f, -1.f, -1.f,  -1.f, -1.f,  1.f,   1.f, -1.f,  1.f
        };
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);
        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVerts), skyboxVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }

    void Renderer3D::DrawSkybox(glm::mat4 view, glm::mat4 proj,
                                glm::vec3 sunDir, float timeOfDay, bool hasLightingNode)
    {
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxProgram);
        glm::mat4 staticView = glm::mat4(glm::mat3(view));
        setMat4(skyboxProgram, "view",       staticView);
        setMat4(skyboxProgram, "projection", proj);
        set3f (skyboxProgram, "sunDir",     sunDir);
        set1f (skyboxProgram, "timeOfDay",  timeOfDay);
        set1i (skyboxProgram, "hasLightingNode", hasLightingNode ? 1 : 0);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);
    }

    void Renderer3D::InitGrid()
    {
        std::string vg = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/gridVertSrc.glsl"));
        std::string fg = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/gridFragSrc.glsl"));
        gridProgram = linkProgram(vg.c_str(), fg.c_str());

        float verts[] = { 1,1,0, -1,-1,0, -1,1,0, -1,-1,0, 1,1,0, 1,-1,0 };
        glGenVertexArrays(1, &gridVAO);
        unsigned int vbo;
        glGenBuffers(1, &vbo);
        glBindVertexArray(gridVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
        glBindVertexArray(0);
    }

    void Renderer3D::DrawGrid(glm::mat4 view, glm::mat4 proj, glm::vec3 /*cameraPos*/)
    {
        glUseProgram(gridProgram);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        setMat4(gridProgram, "view",       view);
        setMat4(gridProgram, "projection", proj);
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

    void Renderer3D::InitBillboard()
    {
        std::string bvs = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/bbVertSrc.glsl"));
        std::string bfs = LoadShaderFromFile(PathHelper::GetAssetPath("shaders/bbFragSrc.glsl"));
        billboardProgram = linkProgram(bvs.c_str(), bfs.c_str());

        float quad[] = {
            -0.5f,  0.5f, 0.f, 1.f,
            -0.5f, -0.5f, 0.f, 0.f,
             0.5f, -0.5f, 1.f, 0.f,
            -0.5f,  0.5f, 0.f, 1.f,
             0.5f, -0.5f, 1.f, 0.f,
             0.5f,  0.5f, 1.f, 1.f
        };
        glGenVertexArrays(1, &billboardVAO);
        glGenBuffers(1, &billboardVBO);
        glBindVertexArray(billboardVAO);
        glBindBuffer(GL_ARRAY_BUFFER, billboardVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glBindVertexArray(0);
    }

    void Renderer3D::DrawBillboard(unsigned int texID, glm::vec3 worldPos, float size,
                                   glm::mat4 view, glm::mat4 proj)
    {
        if (!texID) return;
        glUseProgram(billboardProgram);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);
        glUniform3fv(glGetUniformLocation(billboardProgram, "worldPos"), 1, glm::value_ptr(worldPos));
        glUniform1f (glGetUniformLocation(billboardProgram, "size"), size);
        setMat4(billboardProgram, "view",       view);
        setMat4(billboardProgram, "projection", proj);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        set1i(billboardProgram, "icon", 0);
        glBindVertexArray(billboardVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
        glEnable(GL_CULL_FACE);
    }

}