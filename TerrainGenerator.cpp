#include "TerrainGenerator.h"
#include <cmath>
#include <iostream>

TerrainGenerator::TerrainGenerator(int gridSize) : gridSize(gridSize) {}

float TerrainGenerator::GetHeight(float x, float z) {
    return FBM(glm::vec2(x, z)) * 15.0f;
}

std::vector<float> TerrainGenerator::GenerateHeightMap() {
    std::vector<float> heightMap(gridSize * gridSize);

    for (int row = 0; row < gridSize; row++) {
        for (int col = 0; col < gridSize; col++) {
            float x = static_cast<float>(col) / gridSize;
            float z = static_cast<float>(row) / gridSize;
            heightMap[row * gridSize + col] = GetHeight(x, z);
        }
    }
    return heightMap;
}

// Implementace Perlin Noise
float TerrainGenerator::PerlinNoise(glm::vec2 pos) {
    glm::ivec2 cell = glm::floor(pos);
    glm::vec2 localPos = glm::fract(pos);

    glm::vec2 fadeXY = glm::vec2(Fade(localPos.x), Fade(localPos.y));

    int h00 = Hash(cell.x, cell.y);
    int h10 = Hash(cell.x + 1, cell.y);
    int h01 = Hash(cell.x, cell.y + 1);
    int h11 = Hash(cell.x + 1, cell.y + 1);

    float n00 = Grad(h00, localPos.x, localPos.y);
    float n10 = Grad(h10, localPos.x - 1.0f, localPos.y);
    float n01 = Grad(h01, localPos.x, localPos.y - 1.0f);
    float n11 = Grad(h11, localPos.x - 1.0f, localPos.y - 1.0f);

    float nx0 = Lerp(n00, n10, fadeXY.x);
    float nx1 = Lerp(n01, n11, fadeXY.x);
    return Lerp(nx0, nx1, fadeXY.y);
}

// Pomocné funkce
float TerrainGenerator::Fade(float t) {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float TerrainGenerator::Lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Výběr náhodného směru gradientu na základě hash hodnoty
float TerrainGenerator::Grad(int hash, float x, float y) {
    int h = hash & 7; // Pouze spodní 3 bity
    float u = (h < 4) ? x : y;
    float v = (h < 2) ? y : x;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Hashovací funkce pro pseudonáhodné hodnoty
int TerrainGenerator::Hash(int x, int y) {
    x = (x << 13) ^ x;
    y = (y << 17) ^ y;
    return ((x * y * 15731 + 789221) ^ (x + y * 1376312589)) & 255;
}

float TerrainGenerator::FBM(glm::vec2 pos) {
    float total = 0.0f;
    float amplitude = 1.0f;
    float frequency = 0.5f;
    float maxValue = 0.0f;

    for (int i = 0; i < 6; i++) { // 6 hladin sumu
        total += PerlinNoise(pos * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;  // Snizujeme amplitudu
        frequency *= 2.0f;   // Zvysujeme frekvenci
    }

    return total / maxValue; // Normalizace
}
