#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>

class Terrain {
public:
    Terrain(int gridSize);
    ~Terrain();
    void Draw();

private:
    unsigned int VAO, VBO, EBO;
    int gridSize;
    void GenerateTerrain();
};

#endif