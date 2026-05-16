#include "heiarchy.h"
#include "Model.h"
#include "Textureloader.h"
#include <cstring>
#include <iostream>
#include <filesystem>
#include "../../utils/PathHelper.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Flux {

static const char* NodeTypeLabel(NodeType t) {
    switch (t) {
        case NodeType::DirectionalLight: return "[Dir]  ";
        case NodeType::PointLight:       return "[Pt]   ";
        case NodeType::SpotLight:        return "[Spot] ";
        case NodeType::SurfaceLight:     return "[Surf] ";
        default:                         return "[Mesh] ";
    }
}

std::shared_ptr<Model> Heiarchy::GetOrLoadModel(const std::string& path) {
    auto it = modelRegistry.find(path);
    if (it != modelRegistry.end()) return it->second;
    auto m = std::make_shared<Model>(path);
    modelRegistry[path] = m;
    return m;
}

SceneNode* Heiarchy::GetLightingNode() {
    for (auto& n : nodes)
        if (n.isLightingNode) return &n;
    return nullptr;
}

void Heiarchy::setup() {
    SceneNode lighting;
    lighting.type            = NodeType::DirectionalLight;
    lighting.name            = "Lighting";
    lighting.isLightingNode  = true;
    lighting.position        = glm::vec3(0.0f, 10.0f, 0.0f);

    float pitch = -45.0f, yaw = 20.0f;
    lighting.rotation = glm::vec3(pitch, yaw, 0.0f);
    glm::quat q = glm::angleAxis(glm::radians(yaw),  glm::vec3(0,1,0))
                * glm::angleAxis(glm::radians(pitch), glm::vec3(1,0,0));
    lighting.light.direction  = glm::normalize(q * glm::vec3(0.f, -1.f, 0.f));
    lighting.light.color      = glm::vec3(1.0f, 0.97f, 0.88f);
    lighting.light.intensity  = 3.5f;
    lighting.light.moonColor     = glm::vec3(0.5f, 0.6f, 0.9f);
    lighting.light.moonIntensity = 1.0f;
    lighting.light.timeOfDay  = 14.0f;
    lighting.light.brightness = 2.0f;

    std::string iconPath = PathHelper::GetAssetPath("assets/icons/l_dir.png");
    if (std::filesystem::exists(iconPath))
        lighting.textureID = TextureLoader::Load(iconPath);

    nodes.push_back(lighting);
    selectedIndex = -1;

    AddModel(PathHelper::GetAssetPath(std::string("assets/models/cube.obj")));
}

std::string Heiarchy::GetUniqueName(const std::string& baseName) {
    std::string name = baseName;

    size_t openParen = name.find_last_of('(');
    size_t closeParen = name.find_last_of(')');
    if (openParen != std::string::npos && closeParen != std::string::npos && closeParen > openParen) {
        name = name.substr(0, openParen - 1); 
    }

    std::string strippedBase = name;
    int counter = 1;

    for (;;) {
        bool clash = false;
        for (const auto& n : nodes) {
            if (n.name == name) { clash = true; break; }
        }
        if (!clash) break;
        name = strippedBase + " (" + std::to_string(counter++) + ")";
    }
    
    return name;
}

void Heiarchy::AddModel(const std::string& path, const std::string& name) {
    SceneNode n;
    n.type  = NodeType::Mesh;
    n.model = GetOrLoadModel(path);
    std::string desired = name.empty() ? std::filesystem::path(path).stem().string() : name;
    n.name  = GetUniqueName(desired);
    nodes.push_back(n);
    selectedIndex = (int)nodes.size() - 1;
}

void Heiarchy::AddLight(NodeType type, const std::string& name) {
    SceneNode n;
    n.type = type;

    auto tryLoadIcon = [&](const char* rel) -> unsigned int {
        std::string p = PathHelper::GetAssetPath(rel);
        if (std::filesystem::exists(p)) return TextureLoader::Load(p);
        return 0;
    };

    switch (type) {
        case NodeType::DirectionalLight:
            n.name      = GetUniqueName(name.empty() ? "Directional Light" : name);
            n.textureID = tryLoadIcon("assets/icons/l_dir.png");
            break;
        case NodeType::PointLight:
            n.name      = GetUniqueName(name.empty() ? "Point Light" : name);
            n.textureID = tryLoadIcon("assets/icons/l_point.png");
            break;
        case NodeType::SpotLight:
            n.name      = GetUniqueName(name.empty() ? "Spot Light" : name);
            n.textureID = tryLoadIcon("assets/icons/l_spot.png");
            break;
        default:
            n.name = GetUniqueName(name.empty() ? "Surface Light" : name);
    }
    nodes.push_back(n);
    selectedIndex = (int)nodes.size() - 1;
}

void Heiarchy::AddCamera(const std::string& name) {
    SceneNode n;
    n.type = NodeType::Camera;
    n.name = GetUniqueName(name.empty() ? "Camera" : name);

    bool hasMain = false;
    for (auto& node : nodes) {
        if (node.isMainCamera) {
            hasMain = true;
            break;
        }
    }
    if (!hasMain) n.isMainCamera = true;

    std::string iconPath = PathHelper::GetAssetPath("assets/icons/camera.png");
    if (std::filesystem::exists(iconPath)) n.textureID = TextureLoader::Load(iconPath);

    std::string modelPath = PathHelper::GetAssetPath("assets/models/camera.obj");
    if (std::filesystem::exists(modelPath)) n.model = GetOrLoadModel(modelPath);

    nodes.push_back(n);
    selectedIndex = (int)nodes.size() -1;
}

void Heiarchy::DrawNode(int index) {
    SceneNode& node = nodes[index];
    std::string uid = "##node" + std::to_string(index);

    if (renamingIndex == index && !node.isLightingNode) {
        ImGui::SetNextItemWidth(-1);
        ImGui::SetKeyboardFocusHere();
        if (ImGui::InputText(("##ren" + uid).c_str(), renameBuffer, sizeof(renameBuffer),
                ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
        {
            if (renameBuffer[0] != '\0') node.name = renameBuffer;
            renamingIndex = -1;
        }
        if (ImGui::IsItemDeactivated()) renamingIndex = -1;
        return;
    }

    bool selected = (selectedIndex == index);
    std::string label = std::string(NodeTypeLabel(node.type)) + node.name + uid;

    if (node.isLightingNode)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.35f, 1.0f));

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_Leaf |
        ImGuiTreeNodeFlags_NoTreePushOnOpen |
        ImGuiTreeNodeFlags_SpanFullWidth;
    if (selected) flags |= ImGuiTreeNodeFlags_Selected;

    if (selected) {
        ImGui::PushStyleColor(ImGuiCol_Header,        ImVec4(0.26f, 0.59f, 0.98f, 0.35f));
        ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.26f, 0.59f, 0.98f, 0.55f));
    }

    ImGui::TreeNodeEx(label.c_str(), flags);

    if (selected) ImGui::PopStyleColor(2);
    if (node.isLightingNode) ImGui::PopStyleColor();

    if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
        selectedIndex = index;

    if (!node.isLightingNode &&
        ImGui::IsItemHovered() &&
        ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        renamingIndex = index;
        std::strncpy(renameBuffer, node.name.c_str(), sizeof(renameBuffer) - 1);
        renameBuffer[sizeof(renameBuffer) - 1] = '\0';
    }

    if (!node.isLightingNode) {
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload("HIER_NODE", &index, sizeof(int));
            ImGui::Text("Moving  %s", node.name.c_str());
            ImGui::EndDragDropSource();
        }
    }

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("HIER_NODE")) {
            int srcIdx = *(const int*)p->Data;
            if (srcIdx != index && srcIdx >= 0 && srcIdx < (int)nodes.size()) {
                if (!nodes[srcIdx].isLightingNode && index != 0) {
                    SceneNode moved = nodes[srcIdx];
                    nodes.erase(nodes.begin() + srcIdx);
                    int insertAt = (srcIdx < index) ? index : index + 1;
                    if (insertAt > (int)nodes.size()) insertAt = (int)nodes.size();
                    nodes.insert(nodes.begin() + insertAt, moved);
                    selectedIndex = insertAt;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem(("##ctx" + uid).c_str())) {
        if (node.isLightingNode) {
            ImGui::TextDisabled("Lighting (locked)");
        } else {
            if (ImGui::MenuItem("Rename")) {
                renamingIndex = index;
                std::strncpy(renameBuffer, node.name.c_str(), sizeof(renameBuffer) - 1);
                renameBuffer[sizeof(renameBuffer) - 1] = '\0';
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Delete")) {
                nodes.erase(nodes.begin() + index);
                if (selectedIndex >= (int)nodes.size())
                    selectedIndex = (int)nodes.size() - 1;
                ImGui::EndPopup();
                return;
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Duplicate")) {
                SceneNode copy    = nodes[index];
                copy.name         = GetUniqueName(copy.name);
                copy.isLightingNode = false;
                nodes.push_back(copy);
                selectedIndex = (int)nodes.size() - 1;
            }
        }
        ImGui::EndPopup();
    }
}

void Heiarchy::renderHeiarchy(const std::filesystem::path& activeProjectPath) {
    ImGui::Begin("Heiarchy");
    if (ImGui::IsWindowHovered()) ImGui::SetWindowFocus();

    ImGui::Text("Scene");
    ImGui::Separator();

    for (int i = 0; i < (int)nodes.size(); i++)
        DrawNode(i);

    float emptyH = std::max(ImGui::GetContentRegionAvail().y, 8.0f);
    ImGui::InvisibleButton("##hierEmpty", ImVec2(ImGui::GetContentRegionAvail().x, emptyH));

    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EXPLORER_FILE")) {
            std::string path(static_cast<const char*>(p->Data));
            std::string ext = std::filesystem::path(path).extension().string();
            if (ext == ".obj" || ext == ".fbx") AddModel(path);
        }
        ImGui::EndDragDropTarget();
    }

    if (ImGui::BeginPopupContextItem("##hierEmpty")) {
        if (ImGui::BeginMenu("Add Object")) {
            if (ImGui::MenuItem("Camera")) {
                AddCamera();
            }
            if (ImGui::BeginMenu("Mesh")) {
                auto tryAdd = [&](const char* rel, const char* addName) {
                    std::string full = PathHelper::GetAssetPath(std::string("assets/models/") + rel);
                    if (!activeProjectPath.empty()) {
                        auto c = activeProjectPath / "models" / rel;
                        if (std::filesystem::exists(c)) full = c.string();
                    }
                    AddModel(full, addName);
                };
                if (ImGui::MenuItem("Cube"))   tryAdd("cube.obj",   "Cube");
                if (ImGui::MenuItem("Sphere")) tryAdd("sphere.obj", "Sphere");
                if (ImGui::MenuItem("Monkey")) tryAdd("monkey.obj", "Monkey");
                if (ImGui::MenuItem("Plane")) tryAdd("plane.obj", "Plane");
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Light")) {
                if (ImGui::MenuItem("Directional Light")) AddLight(NodeType::DirectionalLight);
                if (ImGui::MenuItem("Point Light"))       AddLight(NodeType::PointLight);
                if (ImGui::MenuItem("Spot Light"))        AddLight(NodeType::SpotLight);
                if (ImGui::MenuItem("Surface Light"))     AddLight(NodeType::SurfaceLight);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

}