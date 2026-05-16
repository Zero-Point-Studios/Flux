#pragma once
#include <glad/glad.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "stb_image.h"
#include <SDL3/SDL.h>
#include "../engine/gui/viewport/output.h"

namespace Flux {

    class Output;
    class TextureLoader {
    public:
        static TextureLoader& Get() {
            static TextureLoader instance;
            return instance;
        }

        static unsigned int Load(const std::string& path);
        void Unload(const std::string& path);
        static unsigned int LoadCubemap(std::vector<std::string> faces);
    private:
        static std::unordered_map<std::string, unsigned int> cache;
    };

}