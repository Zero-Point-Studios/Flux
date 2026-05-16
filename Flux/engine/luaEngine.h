#pragma once

#include <lua.hpp>
#include <lua.h>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <string>
#include <iostream>
#include "gui/viewport/output.h"
#include <filesystem>
#include <fstream>

#include <mechanics/Scenenode.h>

#include <SDL3/SDL.h>

namespace Flux {
    class Output;

    class LuaEngine {
        public:
            LuaEngine() = default;
            ~LuaEngine() = default;

            bool isRunning = false;

            void init();
            void step();

            void start();
            void stop();

            void runScript(const std::string& code);

            void runAllScriptsInFolder(const std::string& folderPath);

            std::vector<SceneNode>* activeNodes = nullptr; 
            
            void bindEngineAPI();

        private:
            sol::state lua;
            sol::protected_function luaOnStart;
            sol::protected_function luaOnUpdate;
            sol::protected_function luaOnEnd;

            std::vector<sol::protected_function> m_startFuncs;
            std::vector<sol::protected_function> m_updateFuncs;
            std::vector<sol::environment> m_environments;
    };
}