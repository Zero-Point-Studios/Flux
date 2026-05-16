#include "explorer.h"
#include "imgui.h"
#include <cstring>
#include <iostream>

namespace Flux
{
	std::filesystem::path Explorer::resolveUniqueName(
	    const std::filesystem::path& parentDir,
	    const std::string& baseStem,
	    const std::string& ext) const
	{
		std::filesystem::path candidate = parentDir / (baseStem + ext);
		int counter = 1;
		while (std::filesystem::exists(candidate)) {
			candidate = parentDir /
			    (baseStem + " (" + std::to_string(counter) + ")" + ext);
			++counter;
		}
		return candidate;
	}


	void Explorer::renderExplorer(Viewport& viewport) {
		ImGui::Begin("Explorer");

		if (ImGui::IsWindowHovered()) {
			ImGui::SetWindowFocus();
		}

		static float refreshTimer = 0.0f;
		refreshTimer += ImGui::GetIO().DeltaTime;

		if (refreshTimer > 2.0f && std::filesystem::exists(activeFolderPath)) {
			auto currentTime = std::filesystem::last_write_time(activeFolderPath);
			if (currentTime != lastFolderTime) {
				refreshRequested = true;
				lastFolderTime = currentTime;
			}
			refreshTimer = 0.0f;
		}

		if (ImGui::TreeNodeEx("Project Files", ImGuiTreeNodeFlags_DefaultOpen)) {
			DrawVirtualNodes(projectRoot);
			ImGui::TreePop();
		}

		if (ImGui::IsWindowFocused() &&
		    ImGui::IsMouseReleased(ImGuiMouseButton_Right) &&
		    ImGui::GetIO().MouseDragMaxDistanceSqr[ImGuiMouseButton_Right] < 10.0f)
		{
			ImGui::OpenPopup("ContextMenuExplorer");
		}

		if (ImGui::BeginPopup("ContextMenuExplorer")) {
			if (ImGui::MenuItem("Create a new Project")) {
				std::filesystem::path searchPath = std::filesystem::current_path();
				bool found = false;
				for (int i = 0; i < 5; ++i) {
					auto candidate = searchPath / "templates" /
					    "Project_Templates" / "base_game_folder_lua";
					if (std::filesystem::exists(candidate)) {
						pendingTemplateRoot = candidate;
						found = true;
						break;
					}
					if (searchPath.has_parent_path())
						searchPath = searchPath.parent_path();
					else
						break;
				}
				if (found) {
					std::strncpy(newProjectNameBuf, "NewGame",
					             sizeof(newProjectNameBuf) - 1);
					newProjectNameBuf[sizeof(newProjectNameBuf) - 1] = '\0';
					showNewProjectModal = true;
					ImGui::CloseCurrentPopup();
				} else {
					std::cerr << "ERROR: template folder not found near "
					          << std::filesystem::current_path() << "\n";
				}
			}

			if (ImGui::MenuItem("Open existing project")) {
				auto selection = pfd::select_folder("Select a project folder").result();

				if (!selection.empty()) {
					std::filesystem::path selectedPath(selection);

					activeFolderPath = selectedPath;
					projectRoot.path = selectedPath;
					projectRoot.name = selectedPath.filename().string();
					syncFiles(selectedPath, projectRoot);
					scanForBackups();
				}
			}

			ImGui::EndPopup();
		}

		if (showNewProjectModal) {
			ImGui::OpenPopup("Name Your Project");
			showNewProjectModal = false;
		}

		float modalW = ImGui::GetMainViewport()->Size.x * 0.30f;
		if (modalW < 320.f) modalW = 320.f;

		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(modalW, 0.f), ImGuiCond_Always);

		if (ImGui::BeginPopupModal("Name Your Project", nullptr, ImGuiWindowFlags_NoResize))
		{
			if (projectLocationBuf[0] == '\0') {
				const char* home = std::getenv("HOME");
				if (!home) home = std::getenv("USERPROFILE");

				if (home) {
					std::filesystem::path defaultPath = std::filesystem::path(home) / "FluxProjects";
					std::strncpy(projectLocationBuf, defaultPath.string().c_str(), sizeof(projectLocationBuf)-1);
				}
			}

			ImGui::Text("Project Name:");
			ImGui::InputText("##projname", newProjectNameBuf, sizeof(newProjectNameBuf));

			ImGui::Spacing();
			ImGui::Text("Save Location:");
			ImGui::InputText("##projlocation", projectLocationBuf, sizeof(projectLocationBuf));

			ImGui::SameLine();

			if (ImGui::Button("Browse...")) {
				auto selection = pfd::select_folder("Choose where to save your project").result();

				if (!selection.empty()) {
					std::strncpy(projectLocationBuf, selection.c_str(), sizeof(projectLocationBuf) - 1);
				}
			}

			ImGui::Spacing();
			bool nameEmpty = (newProjectNameBuf[0] == '\0');
			float btnW = (ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

			if (nameEmpty) ImGui::BeginDisabled();

			if (ImGui::Button("Create", ImVec2(120, 0))) {
				std::filesystem::path docsBase = std::filesystem::path(projectLocationBuf);
				std::filesystem::path docsPath = resolveUniqueName(docsBase, std::string(newProjectNameBuf), "");

				try {
					std::filesystem::create_directories(docsPath);
					std::filesystem::copy(pendingTemplateRoot, docsPath, std::filesystem::copy_options::recursive);

					activeFolderPath = docsPath;
					projectRoot.name = docsPath.filename().string();
					syncFiles(docsPath, projectRoot);
				} catch (const std::filesystem::filesystem_error& e) {
					std::cerr << "FAILED: " << e.what() << "\n";
				}
				ImGui::CloseCurrentPopup();
			}

			if (nameEmpty) ImGui::EndDisabled();

			ImGui::SameLine();

			if (ImGui::Button("Cancel", ImVec2(btnW, 0))) {
				ImGui::CloseCurrentPopup();
			}

			ImGui::EndPopup();
		}

		ImGui::End();

		if (refreshRequested) {
			if (!projectRoot.path.empty() &&
			    std::filesystem::exists(projectRoot.path))
			{
				renamingNode = nullptr;
				syncFiles(projectRoot.path, projectRoot);
			}
			refreshRequested = false;
		}

		if (!pathToDelete.empty()) {
			try {
				if (std::filesystem::exists(pathToDelete))
					std::filesystem::remove_all(pathToDelete);
				if (!projectRoot.path.empty() &&
				    std::filesystem::exists(projectRoot.path))
				{
					renamingNode = nullptr;
					syncFiles(projectRoot.path, projectRoot);
				}
			} catch (const std::filesystem::filesystem_error& e) {
				std::cerr << "Delete failed: " << e.what() << "\n";
			}
			pathToDelete = "";
		}
	}


	void Explorer::DrawVirtualNodes(virtualFile& file) {
		std::string uid = "##" + std::to_string(reinterpret_cast<uintptr_t>(&file));

		if (file.name == ".flux" && file.type == fileType::Folder) return;

		if (renamingNode == &file) {
			ImGui::SetNextItemWidth(-1);
			ImGui::SetKeyboardFocusHere();

			bool commit =
			    ImGui::InputText(("##ren" + uid).c_str(),
			                     renameBuffer, sizeof(renameBuffer),
			                     ImGuiInputTextFlags_EnterReturnsTrue |
			                     ImGuiInputTextFlags_AutoSelectAll);

			bool cancelled = ImGui::IsItemDeactivated() &&
			                 !ImGui::IsItemActivated();

			if (commit && renameBuffer[0] != '\0') {
				std::filesystem::path parentDir = file.path.parent_path();
				std::string stem(renameBuffer);
				std::filesystem::path typedPath(stem);
				std::string typedStem = typedPath.stem().string();
				std::string typedExt  = typedPath.extension().string();

				if (file.type == fileType::Folder) {
					typedStem = stem;
					typedExt  = "";
				}
				if (typedExt.empty() && file.type != fileType::Folder)
					typedExt = file.path.extension().string();

				std::filesystem::path newPath =
				    resolveUniqueName(parentDir, typedStem, typedExt);

				if (newPath != file.path) {
					try {
						std::filesystem::rename(file.path, newPath);
					} catch (const std::filesystem::filesystem_error& e) {
						std::cerr << "Rename failed: " << e.what() << "\n";
					}
				}
				renamingNode     = nullptr;
				refreshRequested = true;
			} else if (cancelled) {
				renamingNode = nullptr;
			}
			return;
		}

		if (file.type == fileType::Folder) {
			ImGuiTreeNodeFlags folderFlags =
			    ImGuiTreeNodeFlags_SpanFullWidth |
			    ImGuiTreeNodeFlags_OpenOnArrow;

			bool node_open = ImGui::TreeNodeEx(
			    (file.name + uid).c_str(), folderFlags);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				std::string payloadPath = file.path.string();
				ImGui::SetDragDropPayload("EXPLORER_FILE",
				    payloadPath.c_str(), payloadPath.size() + 1);
				ImGui::Text("Moving  %s/", file.name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload =
				        ImGui::AcceptDragDropPayload("EXPLORER_FILE"))
				{
					std::string srcStr(static_cast<const char*>(payload->Data));
					std::filesystem::path src(srcStr);

					bool srcInsideDst = false;
					{
						auto rel = std::filesystem::relative(file.path, src);
						auto it = rel.begin();
						if (it != rel.end() && it->string() != "..")
							srcInsideDst = true;
					}

					if (std::filesystem::exists(src) &&
					    src.parent_path() != file.path &&
					    !srcInsideDst)
					{
						std::string stem, ext;
						if (std::filesystem::is_directory(src)) {
							stem = src.filename().string();
							ext  = "";
						} else {
							stem = src.stem().string();
							ext  = src.extension().string();
						}
						std::filesystem::path dst =
						    resolveUniqueName(file.path, stem, ext);
						try {
							std::filesystem::rename(src, dst);
							renamingNode     = nullptr;
							refreshRequested = true;
						} catch (const std::filesystem::filesystem_error& e) {
							std::cerr << "Move failed: " << e.what() << "\n";
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			if (ImGui::BeginPopupContextItem()) {
				activeFolderPath = file.path;

				if (ImGui::MenuItem("Rename")) {
					renamingNode = &file;
					std::strncpy(renameBuffer, file.name.c_str(),
					             sizeof(renameBuffer) - 1);
					renameBuffer[sizeof(renameBuffer) - 1] = '\0';
				}
				if (ImGui::MenuItem("Delete Folder"))
					pathToDelete = file.path;

				ImGui::Separator();

				if (ImGui::BeginMenu("Add..")) {
					if (ImGui::MenuItem("Folder"))
						createNewFolder("NewFolder");
					if (ImGui::MenuItem("Script"))
						copyTemplateItem("scripts", "TemplateScript.lua",
						                 "NewScript", ".lua");
					if (ImGui::BeginMenu("Model")) {
						if (ImGui::MenuItem("Add Cube"))
							copyTemplateItem("models", "cube.obj", "Cube", ".obj");
						if (ImGui::MenuItem("Add Sphere"))
							copyTemplateItem("models", "sphere.obj", "Sphere", ".obj");
						ImGui::EndMenu();
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}

			if (node_open) {
				for (auto& child : file.children)
					DrawVirtualNodes(child);
				ImGui::TreePop();
			}

		} else {
			std::string dispName = file.name;

			bool hasBackup = std::find(filesWithBackups.begin(), filesWithBackups.end(), file.path) != filesWithBackups.end();

			bool isCurrentlyOpen = (activeFilePath == file.path);

			if (isCurrentlyOpen && textEditor != nullptr) {
				if (textEditor->IsTextChanged()) {
					isEditorUnsaved = true;
				}
			}

			if ((isCurrentlyOpen && isEditorUnsaved) || hasBackup) {
				dispName += " *";
			}

			std::string uid = "##" + std::to_string(reinterpret_cast<uintptr_t>(&file));
			ImGui::Selectable((dispName + uid).c_str());

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (file.path.extension() == ".lua") {
					if (textEditor != nullptr) {
						activeScriptName = file.name;
						activeFilePath = file.path;

						std::ifstream ifs(file.path);
						if (ifs.is_open()) {
							std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
							
							textEditor->SetText(content); 
							isEditorVisible = true;
							ifs.close();
						}

						ImGui::SetWindowFocus("Text Editor");
					}
				}
			}

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
				std::string payloadPath = file.path.string();
				if (file.type == fileType::Model) {
					ImGui::SetDragDropPayload("MODEL_FILE",
					    payloadPath.c_str(), payloadPath.size() + 1);
				}
				ImGui::SetDragDropPayload("EXPLORER_FILE",
				    payloadPath.c_str(), payloadPath.size() + 1);
				ImGui::Text("Moving  %s", file.name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginPopupContextItem()) {
				if (ImGui::MenuItem("Rename")) {
					renamingNode = &file;
					std::string stemOnly = file.path.stem().string();
					std::strncpy(renameBuffer, stemOnly.c_str(),
					             sizeof(renameBuffer) - 1);
					renameBuffer[sizeof(renameBuffer) - 1] = '\0';
				}
				ImGui::Separator();
				if (ImGui::MenuItem("Delete"))
					pathToDelete = file.path;
				ImGui::EndPopup();
			}
		}
	}

	void Explorer::syncFiles(const std::filesystem::path& path, virtualFile& node)
	{
		node.children.clear();
		node.path = path;

		for (const auto& entry : std::filesystem::directory_iterator(path)) {
			virtualFile child;
			child.name = entry.path().filename().string();
			child.path = entry.path();

			if (entry.is_directory()) {
				child.type = fileType::Folder;
				syncFiles(entry.path(), child);
			} else {
				std::string ext = entry.path().extension().string();
				if      (ext == ".lua")                  child.type = fileType::Script;
				else if (ext == ".txt")                  child.type = fileType::Text;
				else if (ext == ".obj" || ext == ".fbx") child.type = fileType::Model;
			else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga") child.type = fileType::Texture;
				else                                     child.type = fileType::Text;
			}
			node.children.push_back(child);
		}
	}

	void Explorer::copyTemplateItem(const std::string& folderType,
	                                const std::string& templateName,
	                                const std::string& targetBaseName,
	                                const std::string& ext)
	{
		std::filesystem::path searchPath = std::filesystem::current_path();
		std::filesystem::path absoluteTemplatePath;
		bool found = false;

		for (int i = 0; i < 5; ++i) {
			auto p = searchPath / "templates" / "File_Templates" /
			         folderType / templateName;
			if (std::filesystem::exists(p)) {
				absoluteTemplatePath = p;
				found = true;
				break;
			}
			if (searchPath.has_parent_path()) searchPath = searchPath.parent_path();
			else break;
		}

		if (!found) {
			std::cerr << "ERROR: Could not find template: " << templateName << "\n";
			return;
		}

		std::filesystem::path targetPath =
		    resolveUniqueName(activeFolderPath, targetBaseName, ext);

		try {
			std::filesystem::copy_file(absoluteTemplatePath, targetPath,
			    std::filesystem::copy_options::overwrite_existing);
			refreshRequested = true;
			refreshPath      = activeFolderPath;
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "File Delivery Failed: " << e.what() << "\n";
		}
	}

	void Explorer::createNewFolder(const std::string& name) {
		std::filesystem::path targetPath =
		    resolveUniqueName(activeFolderPath, name, "");

		try {
			std::filesystem::create_directories(targetPath);
			refreshRequested = true;
			refreshPath      = activeFolderPath;
		} catch (const std::filesystem::filesystem_error& e) {
			std::cerr << "Folder Creation Failed: " << e.what() << "\n";
		}
	}

	void Explorer::scanForBackups() {
		filesWithBackups.clear();
		std::filesystem::path backupDir = activeFolderPath / ".flux" / "backups";

		if (std::filesystem::exists(backupDir)) {
			for (const auto& entry : std::filesystem::directory_iterator(backupDir)) {
				if (entry.path().extension() == ".tmp") {
					std::string originalName = entry.path().stem().string();
					filesWithBackups.push_back(activeFolderPath / originalName);
				}
			}
		}
	}
}