#include "Terrain.h"
#include <iostream>

Terrain::Terrain(int gridSize) : gridSize(gridSize) {
    GenerateTerrain();
}

Terrain::~Terrain() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Terrain::GenerateTerrain() {
    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;

    // Generování vertexů
    for (int row = 0; row <= gridSize; row++) {
        for (int col = 0; col <= gridSize; col++) {
            float x = static_cast<float>(col) - gridSize / 2.0f;
            float z = static_cast<float>(row) - gridSize / 2.0f;
            float y = 0.0f; // Výška bude určena ve vertex shaderu
            vertices.push_back(glm::vec3(x, y, z));
        }
    }

    // Generování indexů pro trojúhelníky
    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            int topLeft = row * (gridSize + 1) + col;
            int topRight = topLeft + 1;
            int bottomLeft = (row + 1) * (gridSize + 1) + col;
            int bottomRight = bottomLeft + 1;

            // První trojúhelník
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            // Druhý trojúhelník
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    // Generování VAO, VBO a EBO
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);

    // Poslání vertexů do GPU
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

    // Poslání indexů do GPU
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    // Nastavení atributů vertexů (pozice)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Terrain::Draw() {
    glBindVertexArray(VAO);

    glDrawElements(GL_TRIANGLES, gridSize * gridSize * 6, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
}