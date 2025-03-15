#pragma once
#ifndef TERRAIN_H
#define TERRAIN_H

#include <glad/glad.h>
#include <vector>
#include <glm/glm.hpp>
#include "Shader.h"

class Terrain {
public:
    Terrain(int gridSize);
    ~Terrain();
    void Draw();
    void ComputeTerrain();
    void UpdateTerrain(float scale, float edgeSharpness, int biomeCount, float heightScale);

private:
    unsigned int VAO, VBO, EBO;
    unsigned int positionsSSBO, normalsSSBO, maxValuesSSBO;
    unsigned int workGroupSSBO; // SSBO pro sledování WorkGroup hodnot

    Shader computeShader;

    int gridSize;
    void GenerateTerrain();
};

#endif