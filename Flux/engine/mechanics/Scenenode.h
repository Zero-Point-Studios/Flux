#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <vector>
#include <memory>
#include "imgui.h"
#include "ImGuizmo.h"

namespace Flux {

    class Model;

    enum class NodeType {
        Mesh,
        DirectionalLight,
        PointLight,
        SpotLight,
        SurfaceLight,
        Camera
    };

    struct LightData {
        glm::vec3 color       = glm::vec3(1.0f);
        float     intensity   = 1.0f;

        float range           = 10.0f;
        float innerCutoff     = 12.5f;
        float outerCutoff     = 17.5f;

        glm::vec3 direction   = glm::vec3(0.0f, -1.0f, 0.0f);

        float areaWidth       = 1.0f;
        float areaHeight      = 1.0f;

        float timeOfDay       = 14.0f;
        float brightness      = 2.0f;
        float ambientDaytime  = 0.3f;
        float ambientNight    = 0.1f;
        glm::vec3 fogColor    = glm::vec3(0.75f, 0.84f, 1.0f);
        float fogStart        = 0.0f;
        float fogEnd          = 300.0f;
        glm::vec3 colorShift  = glm::vec3(1.0f);

        glm::vec3 moonColor     = glm::vec3(0.55f, 0.65f, 0.85f);
        float     moonIntensity = 0.18f;
    };

    struct SceneNode {
        std::string            name;
        NodeType               type        = NodeType::Mesh;
        std::shared_ptr<Model> model;
        std::string            texturePath;
        unsigned int           textureID   = 0;

        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 rotation = glm::vec3(0.0f);
        glm::vec3 scale    = glm::vec3(1.0f);

        float roughness = 0.7f;
        float metallic  = 0.0f;
        glm::vec3 baseColor = glm::vec3(0.8f, 0.8f, 0.8f);

        int              parentIndex = -1;
        std::vector<int> children;

        LightData light;

        bool isLightingNode = false;
        bool isMainCamera = false;
        bool isLocked = false;

        glm::mat4 GetTransformMatrix() const {
            float m[16];
            float t[3] = { position.x, position.y, position.z };
            float r[3] = { rotation.x, rotation.y, rotation.z };
            float s[3] = { scale.x,    scale.y,    scale.z    };
            ImGuizmo::RecomposeMatrixFromComponents(t, r, s, m);
            return glm::make_mat4(m);
        }
    };

}