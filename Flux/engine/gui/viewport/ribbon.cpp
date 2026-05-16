#include "ribbon.h"
#include "imgui.h"
#include <iostream>

namespace Flux {
	int currentTool = 0;
	bool showSettings = false;
	void Ribbon::renderRibbon() {
		ImGuiViewport* main_viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(main_viewport->Pos);
		ImGui::SetNextWindowSize(ImVec2(main_viewport->Size.x, 55));

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
			| ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoScrollbar
			| ImGuiWindowFlags_NoDocking
			| ImGuiWindowFlags_MenuBar;

		ImGui::Begin("###Ribbon", nullptr, window_flags);
		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

		if (ImGui::BeginMenuBar()) {
			drawFileMenu();
			drawEditMenu();

			ImGui::Separator();

			drawTransformTools();

			ImGui::EndMenuBar();
		}

		ImGui::SetCursorPosX(main_viewport->Size.x * 0.5f - 50.0f);
		drawProjectControls();

		ImGui::End();
	}


	void Ribbon::drawTransformTools() {
		if (ImGui::RadioButton("Move", currentTool == TOOL_MOVE)) { currentTool = TOOL_MOVE; }
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", currentTool == TOOL_ROTATE)) { currentTool = TOOL_ROTATE; }
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", currentTool == TOOL_SCALE)) { currentTool = TOOL_SCALE; }
	}

	void Ribbon::drawFileMenu() {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("New Project")) {}
			if (ImGui::MenuItem("Open...")) {}
			if (ImGui::MenuItem("Save")) {}
			ImGui::EndMenu();
		}
	}

	void Ribbon::drawEditMenu() {
		if (ImGui::BeginMenu("Edit")) {
			if (ImGui::MenuItem("Undo")) {
				if (textEditorPtr) {
					textEditorPtr->Undo(); 
				}
			}
			if (ImGui::MenuItem("Redo")) {
				if (textEditorPtr) {
					textEditorPtr->Redo();
				}
			}
			if (ImGui::MenuItem("Viewport Settings", nullptr, showSettings)) {
				showSettings = !showSettings;
			}
			ImGui::EndMenu();
		}
	}

	void Ribbon::drawProjectControls() {
		if (luaEnginePtr == nullptr || textEditorPtr == nullptr) return;

		if (ImGui::Button(editorLocked ? "Stop" : "Play")) {
			if (!editorLocked) {
				luaEnginePtr->isRunning = true;
				editorLocked = true;
			} else {
				luaEnginePtr->isRunning = false;
				editorLocked = false;
			}

			playToggledFrame = true;
		}
	}
}