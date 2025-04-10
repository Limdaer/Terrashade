#ifndef TERRAIN_H
#define TERRAIN_H

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Shader.h"
#include "stb_image_write.h"
#include <math.h>



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

struct Erosion {
    float erosionRate = 1;
    float depositionRate = 1;
    int numDroplets = 50000;
    float inertia = 0.1;
    float sedimentCapacityFactor = 2.0;
    float minSedimentCapacity = 0.005;
    float erodeSpeed = 0.3;
    float depositSpeed = 0.1;
    float gravity = 4.0;
    float initialWaterVolume = 1.0;
    float initialSpeed = 0.3;
};

struct ChunkDraw {
    int vertexOffset;
    int chunkX;
    int chunkY;
};

class Terrain {
public:
    Terrain(int gridSize, float worldSize);
    ~Terrain();

    void Draw(Shader terrain, glm::mat4 view, glm::mat4 projection, glm::vec3 cameraPos);
    void ComputeTerrain();
    void ComputeNormals();
    void ComputeErosion(Erosion erosion);
    void UpdateTerrain(float scale, float edgeSharpness, float heightScale, int octaves, float persistence, float lacunarity, unsigned int seed);
    void ReadHeightsFromSSBO();
    void DrawWater(Shader& waterShader, float currentFrame, const glm::mat4& view, const glm::mat4& projection);
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
    float worldSize;
    int dropletIdx = 0;
    std::vector<int> chunksToRender;
    std::vector<ChunkDraw> drawOffsets1;
    std::vector<ChunkDraw> drawOffsets2;
    std::vector<ChunkDraw> drawOffsets4;

private:
    void GenerateTerrain();

    GLuint VAO, VBO, EBOLOD1, EBOLOD2, EBOLOD4;
    GLuint resultsSSBO, uniformBuffer, intsSSBO, chunkPosSSBO, 
        drawOffsetSSBO1, drawOffsetSSBO2, drawOffsetSSBO4;
    Shader computeShader;
    Shader erosionShader;
    Shader normalShader;
    Shader erosionApplyShader;
    Uniforms uniforms = { 0 };

    std::vector<uint32_t> biomeIDs;
    std::vector<float> biomeWeights;
    std::vector<float> heights;
};

#endif // TERRAIN_H