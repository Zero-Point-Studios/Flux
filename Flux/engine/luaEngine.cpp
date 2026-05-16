#include "luaEngine.h"

namespace Flux {
    void LuaEngine::bindEngineAPI() {
        lua.new_usertype<glm::vec3>("Vector3",
            sol::constructors<glm::vec3(), glm::vec3(float), glm::vec3(float, float, float)>(),
            "x", &glm::vec3::x,
            "y", &glm::vec3::y,
            "z", &glm::vec3::z
        );

        lua.new_usertype<SceneNode>("SceneNode",
            "name", &SceneNode::name,
            
            "getPosition", [](SceneNode& n) { return std::make_tuple(n.position.x, n.position.y, n.position.z); },
            "setPosition", [](SceneNode& n, float x, float y, float z) { n.position = glm::vec3(x, y, z); },

            "getRotation", [](SceneNode& n) { return std::make_tuple(n.rotation.x, n.rotation.y, n.rotation.z); },
            "setRotation", [](SceneNode& n, float x, float y, float z) { n.rotation = glm::vec3(x, y, z); },

            "getScale", [](SceneNode& n) { return std::make_tuple(n.scale.x, n.scale.y, n.scale.z); },
            "setScale", [](SceneNode& n, float x, float y, float z) { n.scale = glm::vec3(x, y, z); }
        );

        sol::table engineTable = lua.create_table();
        
        engineTable["getNode"] = [this](const std::string& nodeName) -> SceneNode* {
            if (!activeNodes) return nullptr;
            for (auto& node : *activeNodes) {
                if (node.name == nodeName) return &node;
            }
            return nullptr;
        };

        engineTable["destroyNode"] = [this](const std::string& nodeName) {
            if (!activeNodes) return;
            for (auto it = activeNodes->begin(); it != activeNodes->end(); ++it) {
                if (it->name == nodeName) {
                    activeNodes->erase(it);
                    break;
                }
            }
        };

        engineTable["stop"] = [this]() {
            isRunning = false;
        };

        engineTable["getDeltaTime"] = []() -> float {
            return ImGui::GetIO().DeltaTime; 
        };

        lua["Engine"] = engineTable;
    }

    void LuaEngine::init() {
        lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::math);

        lua["print"] = [](sol::variadic_args args) {
            std::string full_msg = "";
            for (auto v : args) {
                std::string s = v.as<std::string>();
                full_msg += s + " ";
            }

            Output::addLog(full_msg);
	    };

        sol::table inputTable = lua.create_table();
        inputTable["isKeyDown"] = [](const std::string& key) -> bool {
            const bool* state = SDL_GetKeyboardState(NULL);
            SDL_Scancode scancode = SDL_GetScancodeFromName(key.c_str());
            return state[scancode] != 0;
        };

        inputTable["anyKey"] = []() -> bool {
            const bool* state = SDL_GetKeyboardState(NULL);
            for (int i = 0; i < SDL_SCANCODE_COUNT; ++i) {
                if (state[i]) {
                    return true;
                }
            }
            return false;
        };

        inputTable["getKeyPressed"] = []() -> std::string {
            const bool* state = SDL_GetKeyboardState(NULL);
            for (int i = 0; i < 512; ++i) {
                if (state[i]) {
                    const char* name = SDL_GetScancodeName((SDL_Scancode)i);
                    if (name) return std::string(name);
                }
            }
            return "";
        };

        inputTable["getMouseX"] = []() -> float {
            float x, y;
            SDL_GetMouseState(&x, &y);
            return x;
        };

        inputTable["getMouseY"] = []() -> float {
            float x, y;
            SDL_GetMouseState(&x, &y);
            return y;
        };

        inputTable["isMouseDown"] = [](int buttonIndex) -> bool {
            Uint32 state = SDL_GetMouseState(NULL, NULL);
            return (state & SDL_BUTTON_MASK(buttonIndex)) != 0; 
        };

        lua["Input"] = inputTable;
    }

   void LuaEngine::runScript(const std::string& code) {
        sol::environment env(lua, sol::create, lua.globals());
        m_environments.push_back(env);

        auto load_result = lua.load(code, "script", sol::load_mode::text);
        if (!load_result.valid()) {
            sol::error err = load_result;
            Output::addLog("LUA SYNTAX ERROR: " + std::string(err.what()));
            return;
        }

        sol::protected_function script = load_result;
        env.set_on(script);
        
        auto exec_result = script();
        if (!exec_result.valid()) {
            sol::error err = exec_result;
            Output::addLog("LUA EXECUTION ERROR: " + std::string(err.what()));
            return;
        }

        sol::protected_function onStart = env["onStart"];
        sol::protected_function onUpdate = env["onUpdate"];

        if (onStart.valid()) m_startFuncs.push_back(onStart);
        if (onUpdate.valid()) m_updateFuncs.push_back(onUpdate);

        if (isRunning && onStart.valid()) {
            auto result = onStart(); 
            
            if (!result.valid()) {
                sol::error err = result;
                Output::addLog("LUA RUNTIME ERROR (onStart): " + std::string(err.what()));
            }
        }

        if (isRunning && onStart.valid()) onStart();
    }

   void LuaEngine::step() {
        if (m_updateFuncs.empty()) return;

        for (auto& updateFunc : m_updateFuncs) {
        auto result = updateFunc();
        if (!result.valid()) {
            sol::error err = result;
            Output::addLog("LUA RUNTIME ERROR: " + std::string(err.what()));
        }
    }
    }

    void LuaEngine::stop() {
        if (luaOnEnd.valid()) {
            luaOnEnd();

            sol::protected_function_result result = luaOnEnd();
        
            if (!result.valid()) {
                sol::error err = result;
                Output::addLog("LUA RUNTIME ERROR (End): " + std::string(err.what()));

                isRunning = false;
                stop();
            }
        }
        
        m_startFuncs.clear();
        m_updateFuncs.clear();
        m_environments.clear();
        isRunning = false;
    }

    void LuaEngine::runAllScriptsInFolder(const std::string& folderPath) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folderPath)) {
            if (entry.path().extension() == ".lua") {
                std::ifstream ifs(entry.path());
                if (ifs.is_open()) {
                    std::string code{(std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>())};
                    runScript(code);
                    ifs.close();
                }
            }
        }
    }
}