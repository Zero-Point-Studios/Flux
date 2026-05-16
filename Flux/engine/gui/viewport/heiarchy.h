#pragma once
#include "imgui.h"
#include "./mechanics/Scenenode.h"
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
#include <unordered_map>

namespace Flux {

    class Model;

    class Heiarchy {
    public:
        std::vector<SceneNode> nodes;
        int selectedIndex = -1;

        void setup();
        void renderHeiarchy(const std::filesystem::path& activeProjectPath);

        void AddModel(const std::string& path, const std::string& name = "");
        void AddLight(NodeType type, const std::string& name = "");

        void AddCamera(const std::string& name = "");

        std::string GetUniqueName(const std::string& baseName);

        SceneNode* GetLightingNode();

    private:
        int  renamingIndex     = -1;
        char renameBuffer[128] = {};

        std::unordered_map<std::string, std::shared_ptr<Model>> modelRegistry;
        std::shared_ptr<Model> GetOrLoadModel(const std::string& path);

        void DrawNode(int index);
    };

}