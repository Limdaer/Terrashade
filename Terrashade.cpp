#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "Camera.h"
#include "Shader.h"
#include "Terrain.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Callback pro změnu velikosti okna, upravuje viewport
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// Inicializace kamery na výchozí pozici
Camera camera(glm::vec3(0.0f, 5.0f, 10.0f)); // Kamera nad terénem

float deltaTime = 0.0f;  // Čas mezi aktuálním a posledním snímkem
float lastFrame = 0.0f;
bool isCursorFree = false; // Kurzorem lze pohybovat jen, když držíme ALT

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


    // Načtení OpenGL funkcí pomocí GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Povolení hloubkového testu
    glEnable(GL_DEPTH_TEST);

    // Načtení shaderů
    Shader terrainShader("Shaders/terrain.vert", "Shaders/terrain.frag");

    // Vytvoření terénu
    Terrain terrain(100); // 100x100 mřížka

    // Hlavní renderovací smyčka
    while (!glfwWindowShouldClose(window)) {
        // Vyčištění obrazovky
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        processInput(window, deltaTime);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        // Výpočet kamerových matic
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

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
        terrainShader.SetVec3("lightColor", glm::vec3(1.0f, 1.0f, 0.9f)); // Teplé světlo
        terrainShader.SetVec3("terrainColor", glm::vec3(0.2f, 0.6f, 0.2f)); // Zelený terén

        // Nastavení ambient, difuzního a spekulárního světla
        terrainShader.SetFloat("ambientStrength", 0.2f);
        terrainShader.SetFloat("diffuseStrength", 0.8f);
        terrainShader.SetFloat("specularStrength", 0.5f);
        terrainShader.SetFloat("shininess", 32.0f);

        // Pozice kamery
        terrainShader.SetVec3("viewPos", camera.Position);

        // Vykreslení terénu
        terrain.Draw();

        // Výměna bufferů a zpracování událostí
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Uvolnění zdrojů a ukončení programu
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}