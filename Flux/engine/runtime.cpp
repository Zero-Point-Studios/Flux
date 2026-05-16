#include "runtime.h"
#include <glad/glad.h>

// "I just finished my homework lol" - Idkthisguy May 14, 2026

namespace Flux
{

void Runtime::Start(const std::string &projectName, const std::filesystem::path &projectPath,
                    std::vector<SceneNode> &copiedNodes)
{
    if (!std::filesystem::exists(projectPath) || !std::filesystem::is_directory(projectPath)) {
        std::cerr << "RUNTIME ERROR: Project folder is missing at: " << projectPath << "\n";
        Output::addLog("RUNTIME ERROR: Project folder is missing at: " + projectPath.string());
        return;
    }
    
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    m_window = SDL_CreateWindow(projectName.c_str(), 1270, 720, SDL_WINDOW_OPENGL);
    m_glContext = SDL_GL_CreateContext(m_window);
    SDL_GL_MakeCurrent(m_window, m_glContext);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        return;
    }

    SDL_GL_SetSwapInterval(1);

    m_renderer.Init();
    m_renderer.InitSkybox();
    
    m_gameNodes.clear();

    for (const auto node : copiedNodes) {
        SceneNode newNode = node;
        if (newNode.type == NodeType::Camera) {
            newNode.model = nullptr;
        }
        if (node.model != nullptr) {
            newNode.model = std::make_shared<Model>(node.model->path); 
        }

        m_gameNodes.push_back(newNode);
    }

    m_luaEngine.activeNodes = &m_gameNodes;

    m_luaEngine.init();
    m_luaEngine.bindEngineAPI();
    m_luaEngine.isRunning = true;

    m_luaEngine.runAllScriptsInFolder(projectPath.string());

    isRunning = true;

    SDL_GL_MakeCurrent(NULL, NULL);
}

void Runtime::SyncCamera(glm::vec3 editorPos, glm::vec3 editorTarget)
{
    this->cameraPos = editorPos;
    this->cameraTarget = editorTarget;
}
void Runtime::Update()
{
    if (!isRunning)
        return;

    SDL_GL_MakeCurrent(m_window, m_glContext);

    float gameTime = 14.0f;
    glm::vec3 gameSunDir = glm::vec3(0, -1, 0);

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT) 
        { 
            m_luaEngine.stop();
            m_luaEngine.isRunning = false;
            
            SDL_GL_MakeCurrent(NULL, NULL); 
            
            this->Stop(); 
            return;
        }
    }
    m_luaEngine.step();

    int w, h;
    SDL_GetWindowSize(m_window, &w, &h);
    glViewport(0, 0, w, h);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 proj = glm::perspective(glm::radians(70.0f), (float)w / (float)h, 0.1f, 2000.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0, 1, 0));

    for (auto& node : m_gameNodes) {
        if (node.isLightingNode) {
            gameTime = node.light.timeOfDay;
            gameSunDir = node.light.direction;
            break;
        }
    }

    for (auto& node : m_gameNodes) {
        if (node.type == NodeType::Camera && node.isMainCamera) {
            glm::mat4 transform = node.GetTransformMatrix();
            glm::vec3 camPos = node.position;
            glm::vec3 camFront = glm::normalize(glm::vec3(transform * glm::vec4(0, 0, -1, 0)));
            glm::vec3 camUp = glm::normalize(glm::vec3(transform * glm::vec4(0, 1, 0, 0)));

            view = glm::lookAt(camPos, camPos + camFront, camUp);
            break;
        }
    }

    m_renderer.DrawSkybox(view, proj, gameSunDir, gameTime, true);

    for (auto &node : m_gameNodes)
    {
        if (node.model != nullptr)
        {
            glm::mat4 model = node.GetTransformMatrix();
            m_renderer.DrawScene(*node.model, node.textureID, model, view, proj, cameraPos, m_gameNodes);
        }
    }

    SDL_GL_SwapWindow(m_window);
    SDL_GL_MakeCurrent(NULL, NULL);
}

void Runtime::Stop()
{
    if (!isRunning) return; 

    m_luaEngine.stop();

    if (m_glContext) {
        SDL_GL_DestroyContext(m_glContext);
        m_glContext = nullptr; 
    }
    
    if (m_window) {
        SDL_DestroyWindow(m_window);
        m_window = nullptr; 
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO); 
    
    isRunning = false;
}

} // namespace Flux