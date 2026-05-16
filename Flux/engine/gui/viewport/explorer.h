#pragma once

#include "imgui.h"
#include "viewport.h"
#include "../texteditor.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include "../lib/portable-file-dialogs.h"

class TextEditor;

namespace Flux {
	enum class fileType { Folder, Script, Text, Model, Texture };

	struct virtualFile
	{
		std::string name;
		fileType type;
		std::filesystem::path path;
		std::vector<virtualFile> children;
	};

	struct creationTask
	{
		std::string defaultName;
		std::string extension;
		bool pending = false;
	};

	class Viewport;

	class Explorer {
	public:
		void renderExplorer(Viewport& viewport);

		bool refreshRequested = false;
		std::filesystem::path refreshPath;
		std::filesystem::path activeFolderPath;
		virtualFile projectRoot = { "Project", fileType::Folder };
		creationTask pendingCreationTask;
		std::filesystem::path pathToDelete = "";

		std::vector<std::filesystem::path> filesWithBackups;

		void scanForBackups();

		::TextEditor* textEditor = nullptr;

		std::filesystem::path activeFilePath;
		std::string activeScriptName;
		bool isEditorVisible = false;
		bool isEditorUnsaved = false;

	private:
		void DrawVirtualNodes(virtualFile& file);
		void syncFiles(const std::filesystem::path& path, virtualFile& node);
		void copyTemplateItem(const std::string& folderType,
							  const std::string& templateName,
							  const std::string& targetBaseName,
							  const std::string& ext);
		void createNewFolder(const std::string& name);

		std::filesystem::path resolveUniqueName(
			const std::filesystem::path& parentDir,
			const std::string& baseStem,
			const std::string& ext) const;

		virtualFile* renamingNode = nullptr;
		char         renameBuffer[256] = {};

		bool showNewProjectModal     = false;
		bool showOpenProjectModal    = false;
		char newProjectNameBuf[256]  = "NewGame";
		std::filesystem::path pendingTemplateRoot;
		std::filesystem::file_time_type lastFolderTime;
		char projectLocationBuf[512] = "";
	};
}