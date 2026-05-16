 

#pragma once

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <filesystem>
#include "./gui/viewport/viewport.h"
#include "./gui/viewport/explorer.h"
#include "./gui/viewport/ribbon.h"
#include "./gui/viewport/output.h"
#include "./gui/viewport/properties.h"
#include "./gui/viewport/heiarchy.h"
#include "./gui/texteditor.h"
#include "luaEngine.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "stb_image.h"

#include "runtime.h"

#include <SDL3/SDL.h> // For runtime

namespace Flux {
	class Window
	{
	public:
		Window(int width, int height, const std::string& title);
		~Window();

		bool shouldClose() const;
		void update();
		void clear(float r, float g, float b, float a);

		GLFWwindow* getNativeWindow() const { return m_window; };

	private:
		std::vector<SceneNode> m_runtimeNodes;

		GLFWwindow* m_window;
		int m_width, m_height;
		std::string m_title;
		Viewport m_viewport;
		Explorer m_explorer;
		Ribbon m_ribbon;
		Output m_output;
		Properties m_properties;
		Heiarchy m_heiarchy;
		TextEditor m_texteditor;
		LuaEngine m_luaEngine;
		Runtime m_runtime;

		void StartRuntimeEngine();
		void StopRuntimeEngine();

		bool m_stoppingRuntime = false;
	};
}