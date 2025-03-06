#pragma once
#ifndef TERRAIN_GENERATOR_H
#define TERRAIN_GENERATOR_H

#include <vector>
#include <glm/glm.hpp>

class TerrainGenerator {
public:
    TerrainGenerator(int gridSize);
    float GetHeight(float x, float z);
    std::vector<float> GenerateHeightMap(); 

private:
    int gridSize;

    // Funkce pro Perlin Noise
    float PerlinNoise(glm::vec2 pos);
    float Fade(float t);
    float Lerp(float a, float b, float t);
    float Grad(int hash, float x, float y);
    int Hash(int x, int y);
    float FBM(glm::vec2 pos);

};

#endif
