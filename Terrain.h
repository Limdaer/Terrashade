#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "stb_image_write.h"



struct alignas(16) Params {
    float fbmFreq;
    float fbmAmp;
    float ridgeFreq;
    float ridgeAmp;
    float voroFreq;
    float voroAmp;
    float morphedvoroFreq;
    float morphedvoroAmp;
    float sandFreq;
    float sandAmp;
    int enabled;
    int padding1, padding2, padding3;
};

struct alignas(16) Output {
    glm::vec4 position;
    glm::vec4 normal;
    unsigned int biomeIDs[3];
    float biomeWeight[3];
    float waterAmount;
    float sedimentAmount;
};


struct Uniforms {
    Params Dunes;
    Params Plains;
    Params Mountains;
    Params Sea;
};

class Terrain {
public:
    Terrain(int gridSize);
    ~Terrain();

    void Draw();
    void ComputeTerrain();
    void ComputeNormals();
    void ComputeErosion();
    void UpdateTerrain(float scale, float edgeSharpness, float heightScale, int octaves, float persistence, float lacunarity);
    void ReadHeightsFromSSBO();
    float GetHeightAt(float worldX, float worldZ);
    void ModifyTerrain(glm::vec3 hitPoint, int mode);
    void UpdateBiomeParams(const Params& dunes, const Params& plains, const Params& mountains, const Params& sea);
    void SaveHeightmapAsPNG(const std::string& filename);
    void SaveBlendWeightsAsPNG(const std::string& filename);
    void SaveBiomeIDsAsPNG(const std::string& filename);
    float radius = 10.0f;
    float strength = 2.0f;
    float sigma = radius / 3.0f;
    int gridSize;

private:
    void GenerateTerrain();

    GLuint VAO, VBO, EBO;
    GLuint resultsSSBO, uniformBuffer;
    Shader computeShader;
    Shader erosionShader;
    Uniforms uniforms = { 0 };

    std::vector<uint32_t> biomeIDs;
    std::vector<float> biomeWeights;
    std::vector<float> heights;
};

#endif // TERRAIN_H