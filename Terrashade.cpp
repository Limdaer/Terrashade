﻿#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Camera.h"
#include "Shader.h"
#include "Terrain.h"
#include "Texture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Skybox.h"
#include <sstream>
#include <vector> 
#include <string>
#include <iomanip>



const char* gl_translate_error(GLenum code)
{
    switch (code)
    {
    case GL_INVALID_ENUM:                  return "INVALID_ENUM";
    case GL_INVALID_VALUE:                 return "INVALID_VALUE";
    case GL_INVALID_OPERATION:             return "INVALID_OPERATION";
    case GL_STACK_OVERFLOW:                return "STACK_OVERFLOW";
    case GL_STACK_UNDERFLOW:               return "STACK_UNDERFLOW";
    case GL_OUT_OF_MEMORY:                 return "OUT_OF_MEMORY";
    case GL_INVALID_FRAMEBUFFER_OPERATION: return "INVALID_FRAMEBUFFER_OPERATION";
    default:                               return "UNKNOWN_ERROR";
    }
}

GLenum _gl_check_error(const char* file, int line)
{
    GLenum errorCode = 0;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        const char* error = gl_translate_error(errorCode);
        printf("GL error %s | %s (%d)\n", error, file, line);
    }
    return errorCode;
}

#define gl_check_error() _gl_check_error(__FILE__, __LINE__) 

void gl_debug_output_func(GLenum source,
    GLenum type,
    unsigned int id,
    GLenum severity,
    GLsizei length,
    const char* message,
    const void* userParam)
{
    // ignore non-significant error/warning codes
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    (void)length;
    (void)userParam;

    const char* log = "LOG_DEBUG";
    switch (severity)
    {
    case GL_DEBUG_SEVERITY_HIGH:
    case GL_DEBUG_SEVERITY_MEDIUM:       log = "LOG_ERROR"; break;
    case GL_DEBUG_SEVERITY_LOW:          log = "LOG_WARN";  break;
    case GL_DEBUG_SEVERITY_NOTIFICATION: log = "LOG_INFO";  break;
    };

    printf("%s GL error (%d): %s\n", log, (int)id, message);
    switch (source)
    {
    case GL_DEBUG_SOURCE_API:             printf("%s Source: API\n", log); break;
    case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   printf("%s Source: Window System\n", log); break;
    case GL_DEBUG_SOURCE_SHADER_COMPILER: printf("%s Source: Shader Compiler\n", log); break;
    case GL_DEBUG_SOURCE_THIRD_PARTY:     printf("%s Source: Third Party\n", log); break;
    case GL_DEBUG_SOURCE_APPLICATION:     printf("%s Source: Application\n", log); break;
    case GL_DEBUG_SOURCE_OTHER:           printf("%s Source: Other\n", log); break;
    };

    switch (type)
    {
    case GL_DEBUG_TYPE_ERROR:               printf("%s Type: Error\n", log); break;
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: printf("%s Type: Deprecated Behaviour\n", log); break;
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  printf("%s Type: Undefined Behaviour\n", log); break;
    case GL_DEBUG_TYPE_PORTABILITY:         printf("%s Type: Portability\n", log); break;
    case GL_DEBUG_TYPE_PERFORMANCE:         printf("%s Type: Performance\n", log); break;
    case GL_DEBUG_TYPE_MARKER:              printf("%s Type: Marker\n", log); break;
    case GL_DEBUG_TYPE_PUSH_GROUP:          printf("%s Type: Push Group\n", log); break;
    case GL_DEBUG_TYPE_POP_GROUP:           printf("%s Type: Pop Group\n", log); break;
    case GL_DEBUG_TYPE_OTHER:               printf("%s Type: Other\n", log); break;
    };
}


static void gl_post_call_gl_callback(const char* name, void* funcptr, int len_args, ...)
 {
    (void)len_args;

    GLenum error_code = glad_glGetError();
    if (error_code != GL_NO_ERROR)
        printf("GL error %s in %s!\n", gl_translate_error(error_code), name);
}

void gl_debug_output_enable()
{
    int flags = 0;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
    {
        printf("GL Debug info enabled\n");
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        glDebugMessageCallback(gl_debug_output_func, NULL);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
    }
    else
        printf("GL Debug info wasnt enabled! Provide appropriate window hint!\n");

    glDebugMessageCallback(gl_debug_output_func, 0);
    glad_set_post_callback(gl_post_call_gl_callback);
}

// Callback pro změnu velikosti okna, upravuje viewport
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Inicializace kamery na výchozí pozici
Camera camera(glm::vec3(0.0f, 10.0f, 20.0f)); // Kamera nad terénem

float deltaTime = 0.0f;  // Čas mezi aktuálním a posledním snímkem
float lastFrame = 0.0f;
bool isCursorFree = false; // Kurzorem lze pohybovat jen, když držíme ALT
bool StartGUI = true;
int editMode = 0;
bool isEditingTerrain = false;
std::vector<Texture> waterNormalTextures;
const float waterFrames = 120.0;
float currentWaterFrame = 0.0f;
bool erosionEnabled = false;


// Callback pro zpracování pohybu myší
void mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    int screenWidth, screenHeight;
    glfwGetWindowSize(window, &screenWidth, &screenHeight);
    double centerX = screenWidth / 2.0;
    double centerY = screenHeight / 2.0;
    if (!isCursorFree)
        camera.ProcessMouseMovement(xpos, ypos, false);

    if (isEditingTerrain && glfwGetKey(window, GLFW_KEY_LEFT_ALT) != GLFW_PRESS) {
        camera.ProcessMouseMovement(xpos, ypos, true);
    }
    //ZMENIT TOTO !!
    if (xpos <= 1 || xpos >= screenWidth - 1 || ypos <= 1 || ypos >= screenHeight - 1) {
        glfwSetCursorPos(window, centerX, centerY);
        camera.lastX = centerX;
        camera.lastY = centerY;
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        editMode = 1;  // Režim zvětšování terénu
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
        editMode = -1;  // Režim snižování terénu
    }
    else if (action == GLFW_RELEASE) {
        editMode = 0;  // Žádná úprava
    }
}

// Zpracování vstupu z klávesnice
void processInput(GLFWwindow* window, float deltaTime) {
    bool boost = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS; // Zrychlení pohybu

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime, boost);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime, boost);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime, boost);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime, boost);
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime, boost);
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime, boost);

    // Přepnutí režimu kurzoru pomocí ALT
    if (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS) {
        if (!isCursorFree) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL); // Povolit kurzor
            isCursorFree = true;
        }
    }
    else {
        if (isCursorFree) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Zakázat kurzor
            isCursorFree = false;
        }
    }
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true); // Zavření aplikace
}

// Inicializace ImGui
void initImGui(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    // Připojení ImGui k GLFW + OpenGL3
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
}

void renderGUI(Terrain& terrain, Shader& water, Shader& shader, GLFWwindow* window, double mouseX, double mouseY) {
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 100), ImVec2(600, 800));
    ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize); // Hlavní okno GUI
    // **SEKCE UPRAV**
    ImGui::Text("Terrain Settings");
    ImGui::Separator();

    ImGui::Checkbox("Edit Terrain", &isEditingTerrain);
    ImGui::SameLine();
    if (ImGui::Button("Save Heightmap")) {
        terrain.ReadHeightsFromSSBO();
        terrain.SaveHeightmapAsPNG("Export/heightmap.png");
        terrain.SaveBiomeIDsAsPNG("Export/biomeids.png");
        terrain.SaveBlendWeightsAsPNG("Export/biomeweights.png");
    }


    // Rezim uprav
    if (isEditingTerrain) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    else if (!isCursorFree) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }

    if (isEditingTerrain) {
        ImGui::SliderFloat("Modify Radius", &terrain.radius, 1.0f, 50.0f);
        ImGui::SliderFloat("Modify Strength", &terrain.strength, -10.0f, 10.0f);
        ImGui::SliderFloat("Modify Sigma", &terrain.sigma, 0.1f, terrain.radius / 2.0f);
    }

    // **SEKCE TERENU**
    ImGui::Text("Terrain Settings");
    ImGui::Separator();

    static bool autoUpdate = false;
    static float terrainScale = 10.0f;
    static float edgeSharpness = 20.0f;
    static float heightScale = 10.0f;
    static int octaves = 8;
    static float persistence = 0.3f;
    static float lacunarity = 2.0f;
    static unsigned int seed = 1337;
    static int seedInput = static_cast<int>(seed);
    static double erosionPeriod = 0.1;
    static double lastErosionTime = 0;
    static Params dunesParams = { 0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  2.0, 0.5, 1 };
    static Params plainsParams = { 0.63, 0.45,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 1 };
    static Params mountainsParams = { 0.0, 0.0,  2.0, 0.8,  0.8, 2.0,  2.0, 2.0,  0.0, 0.0, 1 };
    static Params seaParams = { 0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 1 };
    static Erosion erosion;
    static float grassHeight = 10.0;
    static float rockHeight = 15.0;
    static float blendRange = 5.0;

    ImGui::Text("Erosion settings");
    ImGui::Separator();

    ImGui::SliderInt("numDroplets", &erosion.numDroplets, 1000, 10000000);
    ImGui::SliderFloat("inertia", &erosion.inertia, 0, 1);
    ImGui::SliderFloat("sedimentCapacityFactor", &erosion.sedimentCapacityFactor, 0, 10);
    ImGui::SliderFloat("minSedimentCapacity", &erosion.minSedimentCapacity, 0, 1);
    ImGui::SliderFloat("erodeSpeed", &erosion.erodeSpeed, 0, 10);
    ImGui::SliderFloat("depositSpeed", &erosion.depositSpeed, 0, 10);
    ImGui::SliderFloat("gravity", &erosion.gravity, 0, 100);
    ImGui::SliderFloat("initialWaterVolume", &erosion.initialWaterVolume, 0, 100);
    ImGui::SliderFloat("initialSpeed", &erosion.initialSpeed, 0, 10);
    ImGui::SliderFloat("erosionRate", &erosion.erosionRate, 0, 10);
    ImGui::SliderFloat("depositionRate", &erosion.depositionRate, 0, 10);

    ImGui::InputInt("Seed", &seedInput);
    if (seedInput < 0) seedInput = abs(seedInput); // zamezit záporným hodnotám

    if (ImGui::Button("Apply Seed")) {
        seed = static_cast<unsigned int>(seedInput);
        terrain.UpdateTerrain(terrainScale, edgeSharpness, heightScale, octaves, persistence, lacunarity, seed);
    }
    ImGui::SameLine();
    if (ImGui::Button("Random Seed")) {
        seed = static_cast<unsigned int>(rand());
        seedInput = static_cast<int>(seed);
        terrain.UpdateTerrain(terrainScale, edgeSharpness, heightScale, octaves, persistence, lacunarity, seed);
    }
    ImGui::Checkbox("Auto Update", &autoUpdate);

    bool updated = false;
    updated |= ImGui::SliderFloat("Scale", &terrainScale, 1.0f, 50.0f);
    updated |= ImGui::SliderFloat("Edge Sharpness", &edgeSharpness, 1.0f, 50.0f);
    updated |= ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 200.0f);
    updated |= ImGui::SliderInt("Octaves", &octaves, 1, 10);
    updated |= ImGui::SliderFloat("Persistence", &persistence, 0.1f, 1.0f);
    updated |= ImGui::SliderFloat("Lacunarity", &lacunarity, 1.0f, 4.0f);

    if (updated)
        erosionEnabled = false;

    if (StartGUI) {
        terrain.UpdateTerrain(terrainScale, edgeSharpness, heightScale, octaves, persistence, lacunarity, seed);
    }

    if (updated && autoUpdate) {
        terrain.UpdateTerrain(terrainScale, edgeSharpness, heightScale, octaves, persistence, lacunarity, seed);
    }

    if (erosionEnabled) {
        if (ImGui::Button("Disable Erosion")) {
            erosionEnabled = false;
            std::cout << "Eroze vypnuta.\n";
        }
    }
    else {
        if (ImGui::Button("Enable Erosion")) {
            erosionEnabled = true;
            std::cout << "Eroze zapnuta.\n";
        }
    }

    double now = glfwGetTime();
    if (erosionEnabled && now - lastErosionTime > erosionPeriod) {
        terrain.ComputeErosion(erosion);
        terrain.ComputeNormals();
        lastErosionTime = now;
    }

    if (!autoUpdate)
        ImGui::SameLine();

    if (ImGui::Button("Reset Terrain")) {
        erosionEnabled = false;
        terrainScale = 10.0f;
        edgeSharpness = 20.0f;
        heightScale = 10.0f;
        octaves = 8;
        persistence = 0.3f;
        lacunarity = 2.0f;
        terrain.UpdateTerrain(terrainScale, edgeSharpness, heightScale, octaves, persistence, lacunarity, seed);
        dunesParams = { 0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  2.0, 0.5 , 1 };
        plainsParams = { 0.4, 0.5,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0,  0.0, 0.0, 1 };
        mountainsParams = { 0.0, 0.0,  2.0, 0.8,  0.8, 2.0,  2.0, 2.0,  0.0, 0.0, 1 };
        terrain.UpdateBiomeParams(dunesParams, plainsParams, mountainsParams, seaParams);

    }

    ImGui::Separator();
    ImGui::Text("Biome Settings");


    bool biomeUpdated = false;

    bool enabledSea = seaParams.enabled;
    if (ImGui::Checkbox("Enable Sea", &enabledSea)) {
        seaParams.enabled = enabledSea ? 1 : 0;
        biomeUpdated = true;
    }

    bool enabledDunes = dunesParams.enabled;
    if (ImGui::Checkbox("Enable Dunes", &enabledDunes)) {
        dunesParams.enabled = enabledDunes ? 1 : 0;
        biomeUpdated = true;
    }

    biomeUpdated |= ImGui::SliderFloat("Dunes FBM Freq", &dunesParams.fbmFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes FBM Amp", &dunesParams.fbmAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Ridge Freq", &dunesParams.ridgeFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Ridge Amp", &dunesParams.ridgeAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Voronoi Freq", &dunesParams.voroFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Voronoi Amp", &dunesParams.voroAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Morphed Freq", &dunesParams.morphedvoroFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Morphed Amp", &dunesParams.morphedvoroAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Sand Freq", &dunesParams.sandFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Dunes Sand Amp", &dunesParams.sandAmp, 0.0f, 5.0f);

    bool enabledPlains = plainsParams.enabled;
    if (ImGui::Checkbox("Enable Plains", &enabledPlains)) {
        plainsParams.enabled = enabledPlains ? 1 : 0;
        biomeUpdated = true;
    }
    biomeUpdated |= ImGui::SliderFloat("Plains FBM Freq", &plainsParams.fbmFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains FBM Amp", &plainsParams.fbmAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Ridge Freq", &plainsParams.ridgeFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Ridge Amp", &plainsParams.ridgeAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Voronoi Freq", &plainsParams.voroFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Voronoi Amp", &plainsParams.voroAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Morphed Freq", &plainsParams.morphedvoroFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Morphed Amp", &plainsParams.morphedvoroAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Sand Freq", &plainsParams.sandFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Plains Sand Amp", &plainsParams.sandAmp, 0.0f, 5.0f);

    bool enabledMountains = mountainsParams.enabled;
    if (ImGui::Checkbox("Enable Mountains", &enabledMountains)) {
        mountainsParams.enabled = enabledMountains ? 1 : 0;
        biomeUpdated = true;
    }
    biomeUpdated |= ImGui::SliderFloat("Mountains FBM Freq", &mountainsParams.fbmFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains FBM Amp", &mountainsParams.fbmAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Ridge Freq", &mountainsParams.ridgeFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Ridge Amp", &mountainsParams.ridgeAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Voronoi Freq", &mountainsParams.voroFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Voronoi Amp", &mountainsParams.voroAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Morphed Freq", &mountainsParams.morphedvoroFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Morphed Amp", &mountainsParams.morphedvoroAmp, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Sand Freq", &mountainsParams.sandFreq, 0.0f, 5.0f);
    biomeUpdated |= ImGui::SliderFloat("Mountains Sand Amp", &mountainsParams.sandAmp, 0.0f, 5.0f);

    if (biomeUpdated)
        erosionEnabled = false;
    if ((biomeUpdated && autoUpdate) || StartGUI) {
        terrain.UpdateBiomeParams(dunesParams, plainsParams, mountainsParams, seaParams);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Lighting Settings");

    static float lightIntensity = 1.0f;
    static float ambientStrength = 0.2f;
    static float diffuseStrength = 0.8f;
    static float specularStrength = 0.15f;
    static float shininess = 1.5f;
    static glm::vec3 lightDir = glm::vec3(-1.0f, -1.0f, -1.0f);
    static glm::vec3 lightColor = glm::vec3(0.8f, 1.0f, 0.9f);
    static glm::vec3 waterColor = glm::vec3(0.0f, 0.3f, 0.8f);

    bool updatedFrag = false;
    updatedFrag |= ImGui::SliderFloat("Light Intensity", &lightIntensity, 0.0f, 2.0f);
    updatedFrag |= ImGui::SliderFloat("Ambient Strength", &ambientStrength, 0.0f, 1.0f);
    updatedFrag |= ImGui::SliderFloat("Diffuse Strength", &diffuseStrength, 0.0f, 2.0f);
    updatedFrag |= ImGui::SliderFloat("Specular Strength", &specularStrength, 0.0f, 1.0f);
    updatedFrag |= ImGui::SliderFloat("Shininess", &shininess, 1.0f, 128.0f);
    updatedFrag |= ImGui::SliderFloat("Light Dir X", &lightDir.x, -1.0f, 1.0f);
    updatedFrag |= ImGui::SliderFloat("Light Dir Y", &lightDir.y, -1.0f, 1.0f);
    updatedFrag |= ImGui::SliderFloat("Light Dir Z", &lightDir.z, -1.0f, 1.0f);
    updatedFrag |= ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
    updatedFrag |= ImGui::ColorEdit3("Water Color", glm::value_ptr(waterColor));
    updatedFrag |= ImGui::SliderFloat("Grass height", &grassHeight, 0.0f, 40.0f);
    updatedFrag |= ImGui::SliderFloat("Rock height", &rockHeight, 0.0f, 40.0f);
    updatedFrag |= ImGui::SliderFloat("Blend range", &blendRange, 0.0f, 10.0f);

    if (updatedFrag || StartGUI) {
        shader.Use();
        shader.SetFloat("lightIntensity", lightIntensity);
        shader.SetFloat("ambientStrength", ambientStrength);
        shader.SetFloat("diffuseStrength", diffuseStrength);
        shader.SetFloat("specularStrength", specularStrength);
        shader.SetFloat("shininess", shininess);
        shader.SetVec3("lightDir", glm::normalize(lightDir));
        shader.SetVec3("lightColor", lightColor);
        shader.SetFloat("grassHeight", grassHeight);
        shader.SetFloat("rockHeight", rockHeight);
        shader.SetFloat("blendRange", blendRange);
        water.Use();
        water.SetVec3("lightDir", glm::normalize(lightDir));
        water.SetVec3("viewPos", camera.Position);
        water.SetVec3("fragWorldPos", glm::vec3(0.0f, -1.0f, 0.0f));
        water.SetFloat("lightIntensity", lightIntensity);
        water.SetFloat("ambientStrength", ambientStrength);
        water.SetFloat("diffuseStrength", diffuseStrength);
        water.SetFloat("specularStrength", specularStrength);
        water.SetVec3("waterColor", waterColor);
        StartGUI = false;
    }
    ImGui::End();
}


void cleanupImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

glm::vec3 GetRayFromMouse(float mouseX, float mouseY, int screenWidth, int screenHeight, const glm::mat4& projection, const glm::mat4& view) {
    float ndcX = (2.0f * mouseX) / screenWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * mouseY) / screenHeight; // Otočit osu Y
    glm::vec4 rayClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);

    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, rayEye.z, 0.0f); // Nastavit směr do dálky

    glm::vec3 rayWorld = glm::vec3(glm::inverse(view) * rayEye);
    rayWorld = glm::normalize(rayWorld); // Normalizovat směr

    return rayWorld;
}

bool RayIntersectsTerrain(glm::vec3 rayOrigin, glm::vec3 rayDir, Terrain& terrain, glm::vec3& hitPoint) {
    float stepSize = 0.01f; // Jak velké kroky děláme
    float maxDistance = 500.0f; // Jak daleko testujeme
    glm::vec3 currentPos = rayOrigin;

    // Aktualizace výšek z SSBO, pokud jsou potřeba
    terrain.ReadHeightsFromSSBO();

    for (float t = 0.0f; t < maxDistance; t += stepSize) {
        currentPos = rayOrigin + rayDir * t;
        float terrainHeight = terrain.GetHeightAt(currentPos.x, currentPos.z);

        if (currentPos.y <= terrainHeight) {
            hitPoint = currentPos;
            return true; // Paprsek narazil na terén
        }
    }
    return false; // Žádný průsečík
}

int main() {
    // Inicializace GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Nastavení OpenGL verze a profilu
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    // Vytvoření okna
    GLFWwindow* window = glfwCreateWindow(1600, 1200, "Procedural Terrain", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);


    // Načtení OpenGL funkcí pomocí GLAD
    gladLoadGL();
    gl_debug_output_enable();

    // Povolení hloubkového testu
    glEnable(GL_DEPTH_TEST);

    // Načtení shaderů
    Shader terrainShader("Shaders/terrain.vert", "Shaders/terrain.frag");
    Shader waterShader("Shaders/water.vert", "Shaders/water.frag");


    Texture grassTexture("textures/grass/text.jpg");
    Texture grassNormalTexture("textures/grass/normal.jpg");
    Texture grassRoughnessTexture("textures/grass/rough.jpg");
    Texture grassAoTexture("textures/grass/ao.jpg");

    Texture rockTexture("textures/rock/text.jpg");
    Texture rockNormalTexture("textures/rock/normal.jpg");
    Texture rockRoughnessTexture("textures/rock/rough.jpg");
    Texture rockAoTexture("textures/rock/ao.jpg");

    Texture snowTexture("textures/snow/text.jpg");
    Texture snowNormalTexture("textures/snow/normal.jpg");
    Texture snowRoughnessTexture("textures/snow/rough.jpg");
    Texture snowAoTexture("textures/snow/ao.jpg");

    Texture sandTexture("textures/sand/text.jpg");
    Texture sandNormalTexture("textures/sand/normal.jpg");
    Texture sandRoughnessTexture("textures/sand/rough.jpg");
    Texture sandAoTexture("textures/sand/ao.jpg");

    std::vector<std::string> waterNormalTexturesPaths;
    for (int i = 1; i <= waterFrames; i++) {
        std::ostringstream oss;
        oss << "textures/water/" << std::setw(4) << std::setfill('0') << i << ".png";
        waterNormalTexturesPaths.push_back(oss.str());
    }
    Texture waterNormalTextureArray(waterNormalTexturesPaths, true);

    initImGui(window);

    // Vytvoření terénu
    Terrain terrain(1500, 1500); // 1000x1000 mřížka
    double mouseX, mouseY;

    Skybox skybox;
    Shader skyboxShader("Shaders/skybox.vert", "Shaders/skybox.frag");

    //BACK CULLING
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);


    while (!glfwWindowShouldClose(window)) {
        // Vyčištění obrazovky
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderGUI(terrain, waterShader, terrainShader, window, mouseX, mouseY);


        // Výpočet kamerových matic
        float FOV = glm::radians(90.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(FOV, 800.0f / 600.0f, 0.1f, 10000.0f);
        glm::vec3 rayDir = GetRayFromMouse((float)mouseX, (float)mouseY, 1600, 1200, projection, view);

        glm::vec3 rayOrigin = camera.Position;
        glm::vec3 hitPoint;

        if (editMode != 0 && isEditingTerrain) {
            glm::vec3 hitPoint;
            if (RayIntersectsTerrain(camera.Position, rayDir, terrain, hitPoint)) {
                terrain.ModifyTerrain(hitPoint, editMode);
                terrain.ComputeNormals();
            }
        }
        currentWaterFrame += 0.2f; // Rychlost animace
        if (currentWaterFrame >= waterFrames) currentWaterFrame = 0.0f;
        // Vykreslení vody

        // Použití shaderu
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat3 modelMatrix = glm::mat3(glm::transpose(glm::inverse(model)));
        // Použití shaderu
        terrainShader.Use();
        terrainShader.SetMat4("model", glm::value_ptr(model));
        terrainShader.SetMat4("view", glm::value_ptr(view));
        terrainShader.SetMat4("projection", glm::value_ptr(projection));
        terrainShader.SetMat3("modelMatrix", glm::value_ptr(modelMatrix));
        terrainShader.SetUInt("gridSize", terrain.gridSize);
        terrainShader.SetFloat("gridDx", terrain.worldSize / terrain.gridSize);


        terrainShader.SetFloat("waterFrame", currentWaterFrame);

        // Pozice kamery
        terrainShader.SetVec3("viewPos", camera.Position);


        // Nastavení texturových jednotek pro shader
        glUniform1i(glGetUniformLocation(terrainShader.ID, "grassTexture"), 0);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "grassNormal"), 1);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "grassRough"), 2);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "grassAo"), 3);

        glUniform1i(glGetUniformLocation(terrainShader.ID, "rockTexture"), 4);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "rockNormal"), 5);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "rockRough"), 6);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "rockAo"), 7);

        glUniform1i(glGetUniformLocation(terrainShader.ID, "snowTexture"), 8);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "snowNormal"), 9);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "snowRough"), 10);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "snowAo"), 11);

        glUniform1i(glGetUniformLocation(terrainShader.ID, "sandTexture"), 12);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "sandNormal"), 13);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "sandRough"), 14);
        glUniform1i(glGetUniformLocation(terrainShader.ID, "sandAo"), 15);

        // Aktivace textur před vykreslením
        glActiveTexture(GL_TEXTURE0);
        grassTexture.Bind(0);
        glActiveTexture(GL_TEXTURE1);
        grassNormalTexture.Bind(1);
        glActiveTexture(GL_TEXTURE2);
        grassRoughnessTexture.Bind(2);
        glActiveTexture(GL_TEXTURE3);
        grassAoTexture.Bind(3);
        glActiveTexture(GL_TEXTURE4);
        rockTexture.Bind(4);
        glActiveTexture(GL_TEXTURE5);
        rockNormalTexture.Bind(5);
        glActiveTexture(GL_TEXTURE6);
        rockRoughnessTexture.Bind(6);
        glActiveTexture(GL_TEXTURE7);
        rockAoTexture.Bind(7);
        glActiveTexture(GL_TEXTURE8);
        snowTexture.Bind(8);
        glActiveTexture(GL_TEXTURE9);
        snowNormalTexture.Bind(9);
        glActiveTexture(GL_TEXTURE10);
        snowRoughnessTexture.Bind(10);
        glActiveTexture(GL_TEXTURE11);
        snowAoTexture.Bind(11);
        glActiveTexture(GL_TEXTURE12);
        sandTexture.Bind(12);
        glActiveTexture(GL_TEXTURE13);
        sandNormalTexture.Bind(13);
        glActiveTexture(GL_TEXTURE14);
        sandRoughnessTexture.Bind(14);
        glActiveTexture(GL_TEXTURE15);
        sandAoTexture.Bind(15);

        // Vykreslení terénu
        terrain.Draw(terrainShader,view,projection, camera.Position);
        terrain.DrawWater(waterShader, currentWaterFrame, view, projection);

        // Vykreslení skyboxu
        glDepthFunc(GL_LEQUAL);
        skyboxShader.Use();

        view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // Odstranění posunu kamery
        projection = glm::perspective(glm::radians(45.0f), 1600.0f / 1200.0f, 0.1f, 500.0f);

        skyboxShader.SetMat4("view", glm::value_ptr(view));
        skyboxShader.SetMat4("projection", glm::value_ptr(projection));
        glm::mat4 skyboxModel = glm::mat4(1.0f);
        skyboxModel = glm::translate(skyboxModel, glm::vec3(0.0f, -50.0f, 0.0f)); // Posun dolů o 50 jednotek
        skyboxShader.SetMat4("model", glm::value_ptr(skyboxModel));

        skybox.Draw(skyboxShader);
        glDepthFunc(GL_LESS);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Výměna bufferů a zpracování událostí
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Uvolnění zdrojů a ukončení programu
    cleanupImGui();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
#define GLAD_GL_IMPLEMENTATION
#include <glad/glad.h>