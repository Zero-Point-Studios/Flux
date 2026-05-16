#include "output.h"

namespace Flux
{
void Output::addLog(const std::string &log)
{
    logBuffer += log + "\n";
}

void Output::renderOutput()
{
    ImGui::Begin("Output");

    if (ImGui::Button("Clear Output"))
    {
        logBuffer.clear();
    }

    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0, 0, 0, 1));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), ImGuiChildFlags_None, ImGuiWindowFlags_HorizontalScrollbar))
    {

        float wrapWidth = ImGui::GetContentRegionAvail().x;
        ImVec2 textSize = ImGui::CalcTextSize(logBuffer.c_str(), nullptr, false, wrapWidth);

        ImGui::InputTextMultiline("##ConsoleOut", (char *)logBuffer.c_str(), logBuffer.size() + 1,
                                  ImVec2(-1.0f, textSize.y + 10.0f),
                                  ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoHorizontalScroll);

        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 20.0f)
        {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor();
    ImGui::End();
}
} // namespace Flux