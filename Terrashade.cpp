#include <glad/glad.h>
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

// Callback pro změnu velikosti okna, upravuje viewport
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Inicializace kamery na výchozí pozici
Camera camera(glm::vec3(0.0f, 5.0f, 10.0f)); // Kamera nad terénem

float deltaTime = 0.0f;  // Čas mezi aktuálním a posledním snímkem
float lastFrame = 0.0f;
bool isCursorFree = false; // Kurzorem lze pohybovat jen, když držíme ALT
bool StartGUI = true;

// Callback pro zpracování pohybu myší
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!isCursorFree)
        camera.ProcessMouseMovement(xpos, ypos);
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
        camera.ProcessKeyboard(UP, deltaTime, boost); // Pohyb nahoru (SPACE)
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime, boost); // Pohyb dolů (CTRL)

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

void renderGUI(Terrain& terrain) {
    ImGui::Begin("Terrain Settings");

    static bool autoUpdate = false;
    static float terrainScale = 10.0f;
    static float edgeSharpness = 20.0f;
    static int biomeCount = 3;
    static float heightScale = 10.0f;

    ImGui::Checkbox("Auto Update", &autoUpdate);

    bool updated = false;
    updated |= ImGui::SliderFloat("Scale", &terrainScale, 1.0f, 50.0f);
    updated |= ImGui::SliderFloat("Edge Sharpness", &edgeSharpness, 1.0f, 50.0f);
    updated |= ImGui::SliderInt("Biome Count", &biomeCount, 1, 7);
    updated |= ImGui::SliderFloat("Height Scale", &heightScale, 0.0f, 200.0f);

    if (StartGUI) {
        terrain.UpdateTerrain(terrainScale, edgeSharpness, biomeCount, heightScale);
        StartGUI == false;
    }

    if (updated && autoUpdate) {
        terrain.UpdateTerrain(terrainScale, edgeSharpness, biomeCount, heightScale);
    }

    if (!autoUpdate && ImGui::Button("Regenerate Terrain")) {
        terrain.UpdateTerrain(terrainScale, edgeSharpness, biomeCount, heightScale);
    }

    ImGui::End();
}




void cleanupImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
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

    // Vytvoření okna
    GLFWwindow* window = glfwCreateWindow(1600, 1200, "Procedural Terrain", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSwapInterval(1);


    // Načtení OpenGL funkcí pomocí GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Povolení hloubkového testu
    glEnable(GL_DEPTH_TEST);

    // Načtení shaderů
    Shader terrainShader("Shaders/terrain.vert", "Shaders/terrain.frag");

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

    initImGui(window);
    bool autoUpdate = false;
    float terrainScale = 10.0f;
    float edgeSharpness = 20.0f;

    // Vytvoření terénu
    Terrain terrain(1000); // 1000x1000 mřížka

    while (!glfwWindowShouldClose(window)) {
        // Vyčištění obrazovky
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        renderGUI(terrain);

        // Výpočet kamerových matic
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 500.0f);

        // Použití shaderu
        glm::mat4 model = glm::mat4(1.0f);
        // Použití shaderu
        terrainShader.Use();
        terrainShader.SetMat4("model", glm::value_ptr(model));
        terrainShader.SetMat4("view", glm::value_ptr(view));
        terrainShader.SetMat4("projection", glm::value_ptr(projection));

        // Směr světla
        glm::vec3 lightDir(-1.0f, -1.0f, -1.0f);
        terrainShader.SetVec3("lightDir", glm::normalize(lightDir));

        // Barva světla a terénu
        terrainShader.SetVec3("lightColor", glm::vec3(0.8f, 1.0f, 0.9f)); // Teplé světlo
        terrainShader.SetVec3("terrainColor", glm::vec3(0.2f, 0.6f, 0.2f)); // Zelený terén

        // Nastavení ambient, difuzního a spekulárního světla
        terrainShader.SetFloat("ambientStrength", 0.2f);
        terrainShader.SetFloat("diffuseStrength", 1.3f);
        terrainShader.SetFloat("specularStrength", 0.3f);
        terrainShader.SetFloat("shininess", 32.0f);

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

        // Aktivace textur před vykreslením
        glActiveTexture(GL_TEXTURE0);
        glEnable(GL_TEXTURE_2D);
        grassTexture.Bind(0);
        glActiveTexture(GL_TEXTURE1);
        glEnable(GL_TEXTURE_2D);
        grassNormalTexture.Bind(1);
        glActiveTexture(GL_TEXTURE2);
        glEnable(GL_TEXTURE_2D);
        grassRoughnessTexture.Bind(2);
        glActiveTexture(GL_TEXTURE3);
        glEnable(GL_TEXTURE_2D);
        grassAoTexture.Bind(3);

        glActiveTexture(GL_TEXTURE4);
        glEnable(GL_TEXTURE_2D);
        rockTexture.Bind(4);
        glActiveTexture(GL_TEXTURE5);
        glEnable(GL_TEXTURE_2D);
        rockNormalTexture.Bind(5);
        glActiveTexture(GL_TEXTURE6);
        glEnable(GL_TEXTURE_2D);
        rockRoughnessTexture.Bind(6);
        glActiveTexture(GL_TEXTURE7);
        glEnable(GL_TEXTURE_2D);
        rockAoTexture.Bind(7);

        glActiveTexture(GL_TEXTURE8);
        glEnable(GL_TEXTURE_2D);
        snowTexture.Bind(8);
        glActiveTexture(GL_TEXTURE9);
        glEnable(GL_TEXTURE_2D);
        snowNormalTexture.Bind(9);
        glActiveTexture(GL_TEXTURE10);
        glEnable(GL_TEXTURE_2D);
        snowRoughnessTexture.Bind(10);
        glActiveTexture(GL_TEXTURE11);
        glEnable(GL_TEXTURE_2D);
        snowAoTexture.Bind(11);


        // Vykreslení terénu
        terrain.Draw();

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