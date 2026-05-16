#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include "Model.h"
#include "./mechanics/Scenenode.h"
#include <SDL3/SDL.h>
#include "gui/viewport/output.h"

namespace Flux {

    class Renderer3D {
    public:
        void Init();

        void InitShadowMap(int resolution = 4096);
        void DrawDepthPass(const std::vector<SceneNode>& nodes, glm::vec3 lightDir);

        void DrawScene(Model& model, unsigned int overrideTexID,
                       glm::mat4 modelMatrix, glm::mat4 view, glm::mat4 proj,
                       glm::vec3 cameraPos,
                       const std::vector<SceneNode>& lights,
                       float alpha       = 1.0f,
                       float roughness   = 0.7f,
                       float metallic    = 0.0f,
                       float timeOfDay   = 14.0f,
                       glm::vec3 baseColor = glm::vec3(-1.f)); // -1 = use mesh mtl

        void DrawBillboard(unsigned int texID, glm::vec3 worldPos, float size,
                           glm::mat4 view, glm::mat4 proj);

        void InitGrid();
        void DrawGrid(glm::mat4 view, glm::mat4 proj, glm::vec3 cameraPos);

        void InitSkybox();
        void DrawSkybox(glm::mat4 view, glm::mat4 proj,
                        glm::vec3 sunDir,
                        float timeOfDay      = 14.0f,
                        bool  hasLightingNode = true);

        bool isSelected;

    private:
        unsigned int shaderProgram    = 0;
        unsigned int billboardProgram = 0;
        unsigned int billboardVAO     = 0;
        unsigned int billboardVBO     = 0;
        unsigned int skyboxProgram    = 0;
        unsigned int skyboxVAO        = 0;
        unsigned int skyboxVBO        = 0;
        unsigned int gridProgram      = 0;
        unsigned int gridVAO          = 0;

        unsigned int shadowFBO        = 0;
        unsigned int shadowDepthTex   = 0;
        unsigned int depthProgram     = 0;
        int          shadowResolution = 4096;
        bool         shadowReady      = false;
        glm::mat4    lightSpaceMatrix = glm::mat4(1.f);

        void InitBillboard();
    };

}