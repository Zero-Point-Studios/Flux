#include "viewport.h"
#include "../../utils/pathHelper.h"
#include "3DRenderer.h"
#include "Model.h"
#include "OpenGLManager.h"
#include "heiarchy.h"
#include <glm/gtx/string_cast.hpp>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <algorithm>
#include <cctype>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Flux
{

extern int currentTool;
extern bool showSettings;

void Viewport::Init()
{
    glManager = std::make_unique<OpenGLManager>();
    renderer = std::make_unique<Renderer3D>();
    camera = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 15.0f));
    glManager->Init(1280, 720);
    renderer->Init();
    renderer->InitGrid();
    renderer->InitSkybox();
    renderer->InitShadowMap(4096);
}

bool Viewport::CheckSphereHit(glm::vec3 ro, glm::vec3 rd, glm::vec3 center, float radius)
{
    glm::vec3 oc = ro - center;
    float b = glm::dot(oc, rd);
    float c = glm::dot(oc, oc) - radius * radius;
    return (b * b - c) > 0.0f;
}

glm::vec3 Viewport::RaycastToGroundPlane(ImVec2 mousePos, ImVec2 imagePos, ImVec2 sz, glm::mat4 proj, glm::mat4 view)
{
    float ndcX = ((mousePos.x - imagePos.x) / sz.x) * 2.f - 1.f;
    float ndcY = 1.f - ((mousePos.y - imagePos.y) / sz.y) * 2.f;
    glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    glm::vec3 rayDir = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 ro = camera->Position;
    float denom = rayDir.y;
    if (std::abs(denom) < 1e-6f)
        return ro + rayDir * 10.f;
    float t = -ro.y / denom;
    if (t < 0.f)
        t = 0.f;
    return ro + rayDir * t;
}

void Viewport::HandleObjectSelection(ImVec2 mousePos, ImVec2 sz, glm::mat4 proj, glm::mat4 view, Heiarchy &heiarchy)
{
    float ndcX = (2.f * mousePos.x) / sz.x - 1.f;
    float ndcY = 1.f - (2.f * mousePos.y) / sz.y;
    glm::vec4 rayClip(ndcX, ndcY, -1.f, 1.f);
    glm::vec4 rayEye = glm::inverse(proj) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.f, 0.f);
    glm::vec3 rayWorld = glm::normalize(glm::vec3(glm::inverse(view) * rayEye));
    glm::vec3 ro = camera->Position;

    int best = -1;
    float bestDist = 999999.f;
    for (int i = 0; i < (int)heiarchy.nodes.size(); i++)
    {
        if (heiarchy.nodes[i].isLightingNode || heiarchy.nodes[i].isLocked)
            continue;
        float r = glm::length(heiarchy.nodes[i].scale) * 0.75f;
        if (r < 0.3f)
            r = 0.3f;
        if (CheckSphereHit(ro, rayWorld, heiarchy.nodes[i].position, r))
        {
            float d = glm::distance(ro, heiarchy.nodes[i].position);
            if (d < bestDist)
            {
                bestDist = d;
                best = i;
            }
        }
    }

    heiarchy.selectedIndex = (best == heiarchy.selectedIndex) ? -1 : best;
}

static ImVec2 WorldToScreen(glm::vec3 worldPos, glm::mat4 view, glm::mat4 proj, ImVec2 imagePos, ImVec2 sz)
{
    glm::vec4 clip = proj * view * glm::vec4(worldPos, 1.f);
    if (clip.w <= 0.f)
        return ImVec2(-1, -1);
    glm::vec3 ndc = glm::vec3(clip) / clip.w;
    float sx = (ndc.x * 0.5f + 0.5f) * sz.x + imagePos.x;
    float sy = (1.f - (ndc.y * 0.5f + 0.5f)) * sz.y + imagePos.y;
    return ImVec2(sx, sy);
}

void Viewport::DrawLightGizmos(Heiarchy &heiarchy, glm::mat4 view, glm::mat4 proj, ImVec2 imagePos, ImVec2 sz)
{
    ImDrawList *dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < (int)heiarchy.nodes.size(); i++)
    {
        SceneNode &node = heiarchy.nodes[i];
        if (node.type == NodeType::Mesh)
            continue;
        if (node.isLightingNode)
            continue;

        bool selected = (heiarchy.selectedIndex == i);
        renderer->DrawBillboard(node.textureID, node.position, 0.5f, view, proj);
        ImVec2 sp = WorldToScreen(node.position, view, proj, imagePos, sz);
        if (sp.x < imagePos.x || sp.x > imagePos.x + sz.x)
            continue;
        if (sp.y < imagePos.y || sp.y > imagePos.y + sz.y)
            continue;

        ImU32 dotCol = selected ? IM_COL32(255, 220, 50, 255) : IM_COL32(255, 200, 50, 200);
        ImU32 arrowCol = selected ? IM_COL32(255, 220, 50, 255) : IM_COL32(255, 200, 50, 180);
        dl->AddCircleFilled(sp, selected ? 6.f : 4.f, dotCol);

        if (node.type == NodeType::DirectionalLight || node.type == NodeType::SpotLight)
        {
            if (!node.isLightingNode)
            {
                glm::vec3 ae = node.position + node.light.direction * 1.5f;
                ImVec2 se = WorldToScreen(ae, view, proj, imagePos, sz);
                if (se.x >= imagePos.x && se.x <= imagePos.x + sz.x && se.y >= imagePos.y && se.y <= imagePos.y + sz.y)
                {
                    dl->AddLine(sp, se, arrowCol, 2.f);
                    float dx = se.x - sp.x, dy = se.y - sp.y;
                    float len = std::sqrt(dx * dx + dy * dy);
                    if (len > 1.f)
                    {
                        float ux = dx / len, uy = dy / len;
                        float hl = 10.f, hw = 5.f;
                        ImVec2 lp{se.x - ux * hl + uy * hw, se.y - uy * hl - ux * hw};
                        ImVec2 rp{se.x - ux * hl - uy * hw, se.y - uy * hl + ux * hw};
                        dl->AddTriangleFilled(se, lp, rp, arrowCol);
                    }
                }
            }
        }
        if (node.type == NodeType::PointLight)
            dl->AddCircle(sp, selected ? 14.f : 10.f, arrowCol, 16, 1.5f);
    }
}

void Viewport::RenderViewport(Heiarchy &heiarchy)
{
    ImGuizmo::BeginFrame();
    ImGuiIO &io = ImGui::GetIO();

    if (showSettings)
    {
        if (ImGui::Begin("Viewport Settings", &showSettings))
        {
            if (ImGui::Checkbox("VSync", &vsyncEnabled))
                glfwSwapInterval(vsyncEnabled ? 1 : 0);
            ImGui::Separator();
            ImGui::Text("Camera");
            ImGui::SliderFloat("Sensitivity", &camera->MouseSensitivity, 0.01f, 0.5f);
            ImGui::SliderFloat("Move Speed", &camera->MovementSpeed, 1.0f, 50.0f);
        }
        ImGui::End();
    }

    ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
    if (ImGui::IsWindowHovered())
        ImGui::SetWindowFocus();

    ImGui::Text("FPS: %.1f", io.Framerate);

    ImVec2 sz = ImGui::GetContentRegionAvail();
    if (sz.x < 1.f)
        sz.x = 1.f;
    if (sz.y < 1.f)
        sz.y = 1.f;
    float aspect = sz.x / sz.y;
    float dt = io.DeltaTime;

    bool winFocused = ImGui::IsWindowFocused();
    bool winHovered = ImGui::IsWindowHovered();

    bool altHeld = ImGui::IsKeyDown(ImGuiKey_LeftAlt) || ImGui::IsKeyDown(ImGuiKey_RightAlt);
    bool lmbDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool rmbDown = ImGui::IsMouseDown(ImGuiMouseButton_Right);

    bool orbitActive = winHovered && altHeld && lmbDown && !ImGuizmo::IsOver();
    bool flyActive = winHovered && rmbDown && !ImGuizmo::IsOver();

    if (orbitActive)
    {
        glm::vec3 focalPoint = camera->Position + camera->Front * 10.0f;
        float dx = io.MouseDelta.x * camera->MouseSensitivity;
        float dy = -io.MouseDelta.y * camera->MouseSensitivity;

        glm::mat4 yawMat = glm::rotate(glm::mat4(1.f), glm::radians(-dx), glm::vec3(0, 1, 0));
        glm::mat4 pitchMat = glm::rotate(glm::mat4(1.f), glm::radians(-dy), camera->Right);

        glm::vec3 offset = camera->Position - focalPoint;
        offset = glm::vec3(pitchMat * glm::vec4(offset, 0.f));
        offset = glm::vec3(yawMat * glm::vec4(offset, 0.f));

        camera->Position = focalPoint + offset;
        camera->Yaw += dx;
        camera->Pitch += dy;
        if (camera->Pitch > 89.f)
            camera->Pitch = 89.f;
        if (camera->Pitch < -89.f)
            camera->Pitch = -89.f;
        camera->ProcessMouseMovement(0.f, 0.f);
    }

    if (flyActive)
    {
        camera->ProcessMouseMovement(io.MouseDelta.x, -io.MouseDelta.y);
    }

    if ((flyActive || (winFocused && rmbDown)) && winFocused)
    {
        float spd = ImGui::IsKeyDown(ImGuiKey_LeftShift) ? 3.0f : 1.0f;
        if (ImGui::IsKeyDown(ImGuiKey_W))
            camera->ProcessKeyboard(FORWARD, dt * spd);
        if (ImGui::IsKeyDown(ImGuiKey_S))
            camera->ProcessKeyboard(BACKWARD, dt * spd);
        if (ImGui::IsKeyDown(ImGuiKey_A))
            camera->ProcessKeyboard(LEFT, dt * spd);
        if (ImGui::IsKeyDown(ImGuiKey_D))
            camera->ProcessKeyboard(RIGHT, dt * spd);
        if (ImGui::IsKeyDown(ImGuiKey_Q))
            camera->ProcessKeyboard(DOWN, dt * spd);
        if (ImGui::IsKeyDown(ImGuiKey_E))
            camera->ProcessKeyboard(UP, dt * spd);
    }

    if (winHovered && io.MouseWheel != 0.f)
    {
        float dolly = io.MouseWheel * camera->MovementSpeed * 0.05f;
        camera->ProcessKeyboard(FORWARD, dolly);
    }

    if (winFocused && ImGui::IsKeyPressed(ImGuiKey_Delete))
    {
        int sel = heiarchy.selectedIndex;
        if (sel >= 0 && sel < (int)heiarchy.nodes.size() && !heiarchy.nodes[sel].isLightingNode)
        {
            heiarchy.nodes.erase(heiarchy.nodes.begin() + sel);
            heiarchy.selectedIndex = std::min(sel, (int)heiarchy.nodes.size() - 1);
        }
    }

    glm::mat4 view = camera->GetViewMatrix();
    glm::mat4 proj = glm::perspective(glm::radians(70.0f), aspect, 0.1f, 2000.f);

    ImVec2 imagePos = ImGui::GetCursorScreenPos();

    glm::vec3 sunDir = glm::vec3(0.f, -1.f, 0.f);
    float timeOfDay = 14.f;
    bool hasLightingNode = false;
    SceneNode *ln = heiarchy.GetLightingNode();
    if (ln)
    {
        const float kDawn = 6.0f, kDusk = 18.0f, kBlend = 1.0f;
        float tod = ln->light.timeOfDay;
        float nightT = 0.0f;
        if (tod >= kDusk + kBlend || tod <= kDawn - kBlend)
        {
            nightT = 1.0f;
        }
        else if (tod > kDusk - kBlend && tod < kDusk + kBlend)
        {
            nightT = (tod - (kDusk - kBlend)) / (2.0f * kBlend);
        }
        else if (tod > kDawn - kBlend && tod < kDawn + kBlend)
        {
            nightT = 1.0f - (tod - (kDawn - kBlend)) / (2.0f * kBlend);
        }
        else
        {
            nightT = 0.0f;
        }

        glm::vec3 moonDir = -ln->light.direction;
        sunDir = ln->light.direction;
        timeOfDay = ln->light.timeOfDay;
        hasLightingNode = true;
    }

    renderer->DrawDepthPass(heiarchy.nodes, sunDir);

    glManager->Resize((int)sz.x, (int)sz.y);
    glManager->Bind();
    glViewport(0, 0, (int)sz.x, (int)sz.y);
    glClearColor(0.07f, 0.07f, 0.09f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderer->DrawSkybox(camera->GetViewMatrix(), proj, sunDir, timeOfDay, hasLightingNode);

    for (auto &node : heiarchy.nodes)
    {
        if (!node.model)
            continue;
        renderer->DrawScene(*node.model, node.textureID, node.GetTransformMatrix(), view, proj, camera->Position,
                            heiarchy.nodes, 1.0f, node.roughness, node.metallic, timeOfDay, node.baseColor);
    }

    if (isDraggingModel && ghostModel)
    {
        glm::mat4 gt = glm::translate(glm::mat4(1.f), ghostPos);
        renderer->DrawScene(*ghostModel, 0, gt, view, proj, camera->Position, heiarchy.nodes, 0.45f);
    }

    if (showGrid)
        renderer->DrawGrid(view, proj, camera->Position);

    glManager->Unbind();

    ImGui::Image(reinterpret_cast<void *>(static_cast<intptr_t>(glManager->GetTexture())), sz, ImVec2(0, 1),
                 ImVec2(1, 0));

    ImGui::Checkbox("Show Grid", &showGrid);

    ImVec2 mousePos = io.MousePos;
    ImVec2 mouseInCanvas(mousePos.x - imagePos.x, mousePos.y - imagePos.y);

    bool itemHovered = ImGui::IsItemHovered();

    if (!ribbonPtr->editorLocked)
    {

        if (itemHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver() && !isDraggingModel)
        {
            HandleObjectSelection(mouseInCanvas, sz, proj, view, heiarchy);
        }

        if (isDraggingModel && itemHovered)
            ghostPos = RaycastToGroundPlane(mousePos, imagePos, sz, proj, view);

        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload *p =
                    ImGui::AcceptDragDropPayload("EXPLORER_FILE", ImGuiDragDropFlags_AcceptBeforeDelivery |
                                                                      ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
            {
                std::string path(static_cast<const char *>(p->Data));
                std::string ext = std::filesystem::path(path).extension().string();

                if (ext == ".obj" || ext == ".fbx")
                {
                    if (!isDraggingModel || ghostPath != path)
                    {
                        ghostPath = path;
                        ghostModel = std::make_shared<Model>(path);
                        isDraggingModel = true;
                    }
                    ghostPos = RaycastToGroundPlane(mousePos, imagePos, sz, proj, view);
                    if (p->IsDelivery())
                    {
                        SceneNode n;
                        n.type = NodeType::Mesh;
                        n.model = ghostModel;
                        n.name = heiarchy.GetUniqueName(std::filesystem::path(path).stem().string());
                        n.position = ghostPos;
                        heiarchy.nodes.push_back(n);
                        heiarchy.selectedIndex = (int)heiarchy.nodes.size() - 1;
                        isDraggingModel = false;
                        ghostModel = nullptr;
                        ghostPath = "";
                    }
                }

                if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
                {
                    if (p->IsDelivery())
                    {
                        int sel = heiarchy.selectedIndex;
                        if (sel >= 0 && sel < (int)heiarchy.nodes.size() && heiarchy.nodes[sel].type == NodeType::Mesh)
                        {
                            heiarchy.nodes[sel].texturePath = path;
                            heiarchy.nodes[sel].textureID = TextureLoader::Load(path);
                            if (heiarchy.nodes[sel].model)
                                heiarchy.nodes[sel].model->SetTexture(heiarchy.nodes[sel].textureID);
                        }
                    }
                }
            }
            else
            {
                if (isDraggingModel)
                {
                    isDraggingModel = false;
                    ghostModel = nullptr;
                    ghostPath = "";
                }
            }
            ImGui::EndDragDropTarget();
        }
        else if (isDraggingModel && !ImGui::GetDragDropPayload())
        {
            isDraggingModel = false;
            ghostModel = nullptr;
            ghostPath = "";
        }

        DrawLightGizmos(heiarchy, view, proj, imagePos, sz);

        ImGuizmo::SetOrthographic(false);
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(imagePos.x, imagePos.y, sz.x, sz.y);
        ImGuizmo::Enable(winFocused || winHovered);

        int sel = heiarchy.selectedIndex;
        if (sel >= 0 && sel < (int)heiarchy.nodes.size() && !heiarchy.nodes[sel].isLightingNode)
        {
            ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
            if (currentTool == 1)
                op = ImGuizmo::ROTATE;
            if (currentTool == 2)
                op = ImGuizmo::SCALE;

            auto &target = heiarchy.nodes[sel];
            glm::mat4 mm = target.GetTransformMatrix();
            ImGuizmo::AllowAxisFlip(false);
            ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), op, ImGuizmo::LOCAL, glm::value_ptr(mm));

            if (ImGuizmo::IsUsing())
            {
                float t[3], r[3], s[3];
                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(mm), t, r, s);
                target.position = {t[0], t[1], t[2]};
                target.scale = {s[0], s[1], s[2]};

                if (target.type == NodeType::Mesh)
                {
                    target.rotation = {r[0], r[1], r[2]};
                }
                else
                {
                    float pitch = r[0], yaw = r[1];
                    float roll = (target.isLightingNode || target.type == NodeType::DirectionalLight) ? 0.0f : r[2];
                    target.rotation = {pitch, yaw, roll};
                    glm::quat q = glm::angleAxis(glm::radians(yaw), glm::vec3(0, 1, 0)) *
                                  glm::angleAxis(glm::radians(pitch), glm::vec3(1, 0, 0)) *
                                  glm::angleAxis(glm::radians(roll), glm::vec3(0, 0, 1));
                    target.light.direction = glm::normalize(q * glm::vec3(0.f, -1.f, 0.f));
                }
            }
        }

        if (winFocused && ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
            io.MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 4.f)
        {
            ImGui::OpenPopup("ViewportCtx");
        }

        if (ImGui::BeginPopup("ViewportCtx"))
        {
            auto tryAdd = [&](const char *rel, const char *addName) {
                std::string full = PathHelper::GetAssetPath(std::string("assets/models/") + rel);
                if (!activeProjectPath.empty())
                {
                    auto c = activeProjectPath / "models" / rel;
                    if (std::filesystem::exists(c))
                        full = c.string();
                }
                heiarchy.AddModel(full, addName);
            };
            if (ImGui::BeginMenu("Add"))
            {
                if (ImGui::BeginMenu("Mesh"))
                {
                    if (ImGui::MenuItem("Cube"))
                        tryAdd("cube.obj", "Cube");
                    if (ImGui::MenuItem("Sphere"))
                        tryAdd("sphere.obj", "Sphere");
                    if (ImGui::MenuItem("Monkey"))
                        tryAdd("monkey.obj", "Monkey");
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Light"))
                {
                    if (ImGui::MenuItem("Point Light"))
                        heiarchy.AddLight(NodeType::PointLight);
                    if (ImGui::MenuItem("Spot Light"))
                        heiarchy.AddLight(NodeType::SpotLight);
                    if (ImGui::MenuItem("Surface Light"))
                        heiarchy.AddLight(NodeType::SurfaceLight);
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }
            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

} // namespace Flux