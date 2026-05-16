# Flux
Flux is a versatile, high-performance game engine for 3D (and soon 2D).

---

## Planned

- 2D support
- Scripting support (Lua and C/C++)
- More graphics APIs (Vulkan, DirectX, SDL, and etc.)
- More platforms (Linux and MacOS)
- More features (physics, audio, animation, and etc.)

## Current status

The engine is currently in early development and is not yet ready for production use. However, the core architecture is in place, and the following features are currently implemented:
- 3D rendering (OpenGL 4.1+)
- Viewport camera movement
- Basic scene management
- Model transformation (translation, rotation, and scaling)
- "Decent" lighting

## Getting started
Flux gonna be cross platform on release (Except for macOS which is still in development)
    
| Windows (.zip)                                          | Linux (AppImage)                                                                                                              | Linux (.deb)                                                        |
| :------------------------------------------------------ | :---------------------------------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------ |
| 1. Download the `.zip` from [Releases](../../releases). | 1. Download the `.AppImage` from [Releases](../../releases).                                                                  | 1. Download the `.deb` from [Releases](../../releases).             |
| 2. Extract the folder to wherever you want it.          | 2. Right-click the file -> Properties -> **Allow executing as program**. (or use `chmod +x Flux_Engine.AppImage` in terminal) | 2. Open your terminal in the download folder.                       |
| 3. Run `Flux.exe` to start the engine.                  | 3. Double-click to run (or use `./Flux_Engine.AppImage` in terminal).                                                         | 3. Run `sudo dpkg -i FluxEngine-1.0.0.deb`.                         |
|                                                         | **Note:** If it fails to launch, install `fuse2` or `fuse3` (e.g., `sudo pacman -S fuse2` on Arch).                           | 4. Run `sudo apt install -f` to fix dependencies, then type `Flux`. |

## Sneak peeks

![Screenshot](cool/FluxScreenshot.png)
*The first screenshot of the engine showing the 3D grid and viewport.*

![Screenshot](cool/LightingSnapshot.png)
*A screenshot for the lighting in Flux (as of May 2. 2026)

## Dependencies
Flux is built on the shoulders of giants. We use modern 2026 techniques and these industry-standard libraries:

- [Dear ImGui](https://github.com/ocornut/imgui) - Bloat-free Graphical User Interface
- [GLFW](https://www.glfw.org/) - Windowing and Input handling
- [GLAD](https://glad.dav1d.de/) - OpenGL Multi-Language Loader
- [GLM](https://glm.g-truc.net/0.9.9/index.html) - OpenGL Mathematics
- [Assimp](https://www.assimp.org/) - Open Asset Import Library (3D Models)
- [stb_image](https://github.com/nothings/stb) - Single-file image loading
- [ImGuizmo](https://github.com/cedricguillemet/imguizmo) - Manipulate objects directly in the viewport
- [LinuxDeploy](https://github.com/linuxdeploy/linuxdeploy) - Used to make AppImages for Linux
- [ImGuiColorTextEdit](https://github.com/BalazsJako/ImGuiColorTextEdit) - Credits to BalazsJako for making this, ImGuiColorTextEdit is now the foundation of the text editor and has solved one of my biggest problems making this project.

## Top Contributors

Small party, I know ;-;

- [@Idkthisguy](https://github.com/Idkthisguy) - Creator and lead developer ([Zero Point Studio](https://github.com/Zero-Point-Studio))
