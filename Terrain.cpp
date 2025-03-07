#include "Terrain.h"
#include <iostream>

Terrain::Terrain(int gridSize) : gridSize(gridSize), computeShader("Shaders/Terrain.comp") {
    GenerateTerrain();
    ComputeTerrain();
}


Terrain::~Terrain() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &normalsSSBO);
    glDeleteBuffers(1, &positionsSSBO);
}

void Terrain::GenerateTerrain() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // SSBO pro pozice (x, y, z)
    glGenBuffers(1, &positionsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, positionsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, positionsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // SSBO pro normály
    glGenBuffers(1, &normalsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, normalsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, normalsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // EBO pro indexy trojúhelníků
    std::vector<unsigned int> indices;
    for (int row = 0; row < gridSize - 1; row++) {
        for (int col = 0; col < gridSize - 1; col++) {
            int topLeft = row * gridSize + col;
            int topRight = topLeft + 1;
            int bottomLeft = (row + 1) * gridSize + col;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}


void Terrain::Draw() {
    glBindVertexArray(VAO);

    // Připojení SSBO jako zdroje dat pro VAO
    glBindBuffer(GL_ARRAY_BUFFER, positionsSSBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, normalsSSBO);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, gridSize * gridSize * 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}


void Terrain::ComputeTerrain() {
    computeShader.Use();

    glUniform1i(glGetUniformLocation(computeShader.ID, "gridSize"), gridSize);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1); // Vypocty
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Zápis do bufferu před čtením

    glUseProgram(0);
}