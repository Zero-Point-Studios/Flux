#pragma once

#include "imgui.h"
#include <string>
#include <iostream>
#include <vector>
#include "sol/sol.hpp"

namespace Flux {
	class Output {
	public:
		inline static std::string logBuffer;
		
		static void addLog(const std::string& log);
		void renderOutput();
	};
}