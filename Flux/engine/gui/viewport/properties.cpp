#include "properties.h"
#include "heiarchy.h"
#include "Textureloader.h"
#include "Model.h"
#include <cstring>
#include <filesystem>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Flux {
static void BeginTable2Col() {
    ImGui::BeginTable("##t", 2,
        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings);
    ImGui::TableSetupColumn("L", ImGuiTableColumnFlags_WidthFixed,   100.f);
    ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_WidthStretch);
}

static bool DragVec3Row(const char* label, glm::vec3& v, float speed = 0.1f) {
    float a[3] = { v.x, v.y, v.z };
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-1);
    bool c = ImGui::DragFloat3("##v", a, speed, -FLT_MAX, FLT_MAX, "%.3f");
    if (c) v = { a[0], a[1], a[2] };
    ImGui::PopID();
    return c;
}

static bool ColorRow(const char* label, glm::vec3& c) {
    float col[3] = { c.r, c.g, c.b };
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-1);
    bool ch = ImGui::ColorEdit3("##c", col, ImGuiColorEditFlags_Float);
    if (ch) c = { col[0], col[1], col[2] };
    ImGui::PopID();
    return ch;
}

static bool FloatRow(const char* label, float& f,
                     float spd = 0.01f, float mn = 0.f, float mx = FLT_MAX,
                     const char* fmt = "%.3f") {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-1);
    bool c = ImGui::DragFloat("##f", &f, spd, mn, mx, fmt);
    ImGui::PopID();
    return c;
}

static bool SliderRow(const char* label, float& f,
                      float mn = 0.f, float mx = 1.0f,
                      const char* fmt = "%.2f") {
    ImGui::PushID(label);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted(label);
    ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-1);
    bool c = ImGui::SliderFloat("##s", &f, mn, mx, fmt);
    ImGui::PopID();
    return c;
}

static void TextureSlot(const char* label, SceneNode& node) {
    float avail = ImGui::GetContentRegionAvail().x;
    float swatchH = 56.f;

    ImGui::Spacing();
    ImGui::TextUnformatted(label);

    ImGui::PushID(label);
    if (node.textureID != 0) {
        ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(node.textureID)),
                     ImVec2(avail - 60.f, swatchH), ImVec2(0,1), ImVec2(1,0));
    } else {
        float col[3] = { node.baseColor.r, node.baseColor.g, node.baseColor.b };
        ImGui::SetNextItemWidth(avail - 60.f);
        if (ImGui::ColorButton("##bc", ImVec4(col[0], col[1], col[2], 1.f),
                ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoBorder,
                ImVec2(avail - 60.f, swatchH)))
        {
            ImGui::OpenPopup("##bcPicker");
        }
        if (ImGui::BeginPopup("##bcPicker")) {
            ImGui::ColorPicker3("##bcp", col, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_NoSidePreview);
            node.baseColor = { col[0], col[1], col[2] };
            ImGui::EndPopup();
        }
    }

    ImVec2 dropMin = ImGui::GetItemRectMin();
    ImVec2 dropMax = ImGui::GetItemRectMax();

    ImGui::SameLine();
    if (!node.texturePath.empty()) {
        if (ImGui::SmallButton("X##clr")) {
            node.texturePath = "";
            node.textureID   = 0;
            if (node.model) node.model->SetTexture(0);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Clear texture");
    } else {
        ImGui::TextDisabled(" drop\n here");
    }

    ImGui::SetCursorScreenPos(dropMin);
    ImGui::InvisibleButton("##texDrop", ImVec2(dropMax.x - dropMin.x, dropMax.y - dropMin.y));
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* p = ImGui::AcceptDragDropPayload("EXPLORER_FILE")) {
            std::string droppedPath(static_cast<const char*>(p->Data));
            std::string ext = std::filesystem::path(droppedPath).extension().string();
            if (ext==".png"||ext==".jpg"||ext==".jpeg"||ext==".bmp"||ext==".tga") {
                node.texturePath = droppedPath;
                node.textureID   = TextureLoader::Load(droppedPath);
                if (node.model) node.model->SetTexture(node.textureID);
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
        ImGui::GetWindowDrawList()->AddRect(dropMin, dropMax,
            IM_COL32(80, 160, 255, 200), 3.f, 0, 2.f);

    if (!node.texturePath.empty())
        ImGui::TextDisabled("%s", std::filesystem::path(node.texturePath).filename().string().c_str());

    ImGui::PopID();
}

static glm::vec3 DirToEuler(glm::vec3 dir) {
    dir = glm::normalize(dir);
    float pitch = glm::degrees(std::asin(-dir.y));
    float yaw   = glm::degrees(std::atan2(dir.x, -dir.z));
    return glm::vec3(pitch, yaw, 0.f);
}

void Properties::renderProperties(Heiarchy* h) {
    ImGui::Begin("Properties");
    if (ImGui::IsWindowHovered()) ImGui::SetWindowFocus();

    if (!h || h->selectedIndex < 0 || h->selectedIndex >= (int)h->nodes.size()) {
        ImGui::TextDisabled("No object selected.");
        ImGui::End();
        return;
    }

    SceneNode& node = h->nodes[h->selectedIndex];

    if (node.isLightingNode) {
        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.35f, 1.f), "Lighting  [locked]");
        
    } else {
        char nameBuf[128];
        std::strncpy(nameBuf, node.name.c_str(), sizeof(nameBuf) - 1);
        nameBuf[sizeof(nameBuf) - 1] = '\0';
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##name", nameBuf, sizeof(nameBuf)))
            node.name = nameBuf;
    }
    ImGui::Separator();

    if (node.isLightingNode) {
        ImGui::Text("Lighting Properties");
        ImGui::Spacing();

        BeginTable2Col();

        {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("Time of Day");
            ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-1);
            int   h24   = (int)node.light.timeOfDay;
            int   m60   = (int)((node.light.timeOfDay - h24) * 60.f);
            char  disp[16]; snprintf(disp, sizeof(disp), "%02d:%02d", h24, m60);
            ImGui::PushID("tod");
           if (ImGui::DragFloat("##tod", &node.light.timeOfDay, 0.01f, 0.f, 24.f, disp)) {
                float angle = (node.light.timeOfDay - 12.0f) * 15.0f; 
                node.rotation.x = angle;
                
                glm::quat q = glm::angleAxis(glm::radians(node.rotation.y), glm::vec3(0,1,0))
                            * glm::angleAxis(glm::radians(node.rotation.x), glm::vec3(1,0,0));
                node.light.direction = glm::normalize(q * glm::vec3(0.f, -1.f, 0.f));
            }
            ImGui::PopID();
        }

        FloatRow("Brightness",      node.light.brightness,     0.01f, 0.f, 10.f);
        ColorRow("Color",           node.light.color);
        ColorRow("Moon Color",      node.light.moonColor);
        FloatRow("Moon Intensity",  node.light.moonIntensity, 0.01f, 0.f, 100.f);
        ColorRow("ColorShift",      node.light.colorShift);
        FloatRow("Ambient Day",     node.light.ambientDaytime, 0.005f, 0.f, 1.f);
        FloatRow("Ambient Night",   node.light.ambientNight,   0.005f, 0.f, 1.f);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::AlignTextToFramePadding(); ImGui::TextUnformatted("Direction");
        ImGui::TableSetColumnIndex(1); ImGui::SetNextItemWidth(-1);
        {
            float arr[3] = { node.light.direction.x, node.light.direction.y, node.light.direction.z };
            ImGui::PushID("ldir");
            if (ImGui::DragFloat3("##ld", arr, 0.005f, -1.f, 1.f, "%.3f")) {
                node.light.direction = glm::normalize(glm::vec3(arr[0], arr[1], arr[2]));
                node.rotation = DirToEuler(node.light.direction);
            }
            ImGui::PopID();
        }

        ImGui::Separator();
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0); ImGui::Spacing();
        ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted("Fog Color");
        ImGui::TableSetColumnIndex(1); /* handled below */
        ImGui::EndTable();

        BeginTable2Col();
        ColorRow("Fog Color",  node.light.fogColor);
        FloatRow("Fog Start",  node.light.fogStart, 1.f, 0.f, 5000.f);
        FloatRow("Fog End",    node.light.fogEnd,   1.f, 0.f, 5000.f);
        ImGui::EndTable();

        ImGui::End();
        return;
    }

    ImGui::Text("Transform");
    ImGui::Spacing();
    BeginTable2Col();
    DragVec3Row("Position", node.position, 0.1f);
    DragVec3Row("Rotation", node.rotation, 0.5f);
    DragVec3Row("Scale",    node.scale,    0.01f);
    ImGui::EndTable();

    if (node.type == NodeType::Camera) {
        ImGui::Separator();
        ImGui::Text("Camera Settings");
        ImGui::Spacing();
        
        if (ImGui::Checkbox("Main Camera", &node.isMainCamera)) {
            if (node.isMainCamera) {
                for (auto& otherNode : h->nodes) {
                    if (&otherNode != &node) otherNode.isMainCamera = false;
                }
            }
        }
    }

    if (node.type == NodeType::Mesh) {
        ImGui::Separator();
        ImGui::Text("Surface");
        ImGui::Spacing();

        if (node.model)
            ImGui::TextDisabled("  %s", node.model->path.c_str());
        ImGui::Separator();
        TextureSlot("Albedo", node);

        ImGui::Separator();
        ImGui::Text("Material");
        ImGui::Spacing();
        BeginTable2Col();
        SliderRow("Roughness", node.roughness, 0.0f, 1.0f);
        SliderRow("Metallic",  node.metallic,  0.0f, 1.0f);
        ImGui::EndTable();
    }
    else {
        ImGui::Separator();
        const char* lbl =
            node.type == NodeType::DirectionalLight ? "Directional Light" :
            node.type == NodeType::PointLight       ? "Point Light"       :
            node.type == NodeType::SpotLight        ? "Spot Light"        : "Surface Light";
        ImGui::Text("%s", lbl);
        ImGui::Spacing();

        BeginTable2Col();
        ColorRow("Color",     node.light.color);
        FloatRow("Intensity", node.light.intensity, 0.01f, 0.f, 100.f);

        if (node.type == NodeType::DirectionalLight) {
            if (DragVec3Row("Direction", node.light.direction, 0.01f)) {
                node.light.direction = glm::normalize(node.light.direction);
                node.rotation = DirToEuler(node.light.direction);
            }
        }
        if (node.type == NodeType::PointLight)
            FloatRow("Range", node.light.range, 0.1f, 0.f, 1000.f);

        if (node.type == NodeType::SpotLight) {
            if (DragVec3Row("Direction", node.light.direction, 0.01f)) {
                node.light.direction = glm::normalize(node.light.direction);
                glm::vec3 e = DirToEuler(node.light.direction);
                node.rotation.x = e.x; node.rotation.y = e.y;
            }
            FloatRow("Range",        node.light.range,       0.1f, 0.f, 1000.f);
            FloatRow("Inner Cutoff", node.light.innerCutoff, 0.1f, 0.f, 90.f);
            FloatRow("Outer Cutoff", node.light.outerCutoff, 0.1f, 0.f, 90.f);
        }
        if (node.type == NodeType::SurfaceLight) {
            FloatRow("Area Width",  node.light.areaWidth,  0.01f, 0.f, 100.f);
            FloatRow("Area Height", node.light.areaHeight, 0.01f, 0.f, 100.f);
        }
        ImGui::EndTable();
    }
    ImGui::End();
}

}