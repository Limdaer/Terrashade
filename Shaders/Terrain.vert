#version 460 core

struct Output{
    vec4 position;
    vec4 normal;
    uint biomeIDs[3];
    float biomeWeight[3];
};

layout (std430, binding = 0) buffer Buffer {
    Output outputs[];
};

layout (std430, binding = 1) buffer lodLevelsBuffer {
    int lodLevels[];
};

struct ChunkDraw{
    int vertexOffset;
    int chunkX;
    int chunkY;
};


layout (std430, binding = 2) buffer drawOffsetsBuffer {
    ChunkDraw chunkDraws[];
};

#define CHUNK 33
#define ARR3_TO_VEC3(x) vec3(x[0], x[1], x[2])
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform uint gridSize;
uniform float gridDx;
uniform int chunkCount;
uniform mat3 modelMatrix;

out vec3 FragPos;  
out vec3 Normal;
out vec2 TexCoords; // Výstup pro fragment shader
flat out uint biomeID1;
flat out uint biomeID2;
flat out uint biomeID3;
out vec3 Weights;
out float visible;
out int lod;

void main() {
    ChunkDraw chunkDraw = chunkDraws[gl_InstanceID];
    uint index = gl_VertexID + chunkDraw.vertexOffset;
    Output results = outputs[index];
    uint chunkIdx = chunkDraw.chunkX + chunkDraw.chunkY * chunkCount;
    uint chunkW = (chunkDraw.chunkX - 1) + chunkDraw.chunkY * chunkCount;
    uint chunkE = (chunkDraw.chunkX + 1) + chunkDraw.chunkY * chunkCount;
    uint chunkN = chunkDraw.chunkX + (chunkDraw.chunkY + 1) * chunkCount;
    uint chunkS = chunkDraw.chunkX + (chunkDraw.chunkY - 1) * chunkCount;

    int lodLevel = lodLevels[chunkIdx];
    visible = 0;
    uint localX = gl_VertexID % gridSize;
    uint localY = gl_VertexID / gridSize;
    vec4 position = results.position;
    vec4 normal = results.normal;
    vec3 weights = ARR3_TO_VEC3(results.biomeWeight);
    if(localX == 0 && chunkDraw.chunkX != 0){
        int lodNext = lodLevels[chunkW];
        if(lodLevel < lodNext && localY % lodNext != 0){
            visible = 4;
            uint results1 = index - gridSize * lodLevel;
            uint results2 = index + gridSize * lodLevel;
            position = (outputs[results1].position + outputs[results2].position) / 2;
            normal = (outputs[results1].normal + outputs[results2].normal) / 2;
            weights = (ARR3_TO_VEC3(outputs[results1].biomeWeight) + ARR3_TO_VEC3(outputs[results2].biomeWeight)) / 2;
        }
    }
    if(localX == CHUNK - 1 && chunkDraw.chunkX != chunkCount - 1){
        int lodNext = lodLevels[chunkE];
        if(lodLevel < lodNext && localY % lodNext != 0){
            visible = 4;
            uint results1 = index - gridSize * lodLevel;
            uint results2 = index + gridSize * lodLevel;
            position = (outputs[results1].position + outputs[results2].position) / 2;
            normal = (outputs[results1].normal + outputs[results2].normal) / 2;
            weights = (ARR3_TO_VEC3(outputs[results1].biomeWeight) + ARR3_TO_VEC3(outputs[results2].biomeWeight)) / 2;
        }
    }
    if(localY == 0 && chunkDraw.chunkY != 0){
        int lodNext = lodLevels[chunkS];
        if(lodLevel < lodNext && localX % lodNext != 0){
            visible = 4;
            uint results1 = index - 1 * lodLevel;
            uint results2 = index + 1 * lodLevel;
            position = (outputs[results1].position + outputs[results2].position) / 2;
            normal = (outputs[results1].normal + outputs[results2].normal) / 2;
            weights = (ARR3_TO_VEC3(outputs[results1].biomeWeight) + ARR3_TO_VEC3(outputs[results2].biomeWeight)) / 2;
        }
    }
    if(localY == CHUNK - 1 && chunkDraw.chunkY != chunkCount - 1){
        int lodNext = lodLevels[chunkN];
        if(lodLevel < lodNext && localX % lodNext != 0){
            visible = 4;
            uint results1 = index - 1 * lodLevel;
            uint results2 = index + 1 * lodLevel;
            position = (outputs[results1].position + outputs[results2].position) / 2;
            normal = (outputs[results1].normal + outputs[results2].normal) / 2;
            weights = (ARR3_TO_VEC3(outputs[results1].biomeWeight) + ARR3_TO_VEC3(outputs[results2].biomeWeight)) / 2;
        }
    }

    // Pozice ve světovém prostoru
    vec4 worldPosition = model * vec4(position.xyz, 1.0);
    // Výpočet finální normály
    vec3 normalProject = normalize(modelMatrix * normal.xyz);

    TexCoords = results.position.xz * 0.1; // Opakování textury každých 10 jednotek

    //Předání biomeID
    biomeID1 = results.biomeIDs[0];
    biomeID2 = results.biomeIDs[1];
    biomeID3 = results.biomeIDs[2];

    // Předání do fragment shaderu
    FragPos = worldPosition.xyz;
    Normal = normalProject;
    Weights = weights;
    gl_Position = projection * view * worldPosition;
}
