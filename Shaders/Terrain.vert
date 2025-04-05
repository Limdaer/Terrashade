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

layout (std430, binding = 1) buffer ChunkPosBuffer {
    int chunkPoses[];
};

layout (std430, binding = 2) buffer drawOffsetsBuffer {
    int drawOffsets[];
};

#define CHUNK 32
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform uint gridSize;
uniform float gridDx;
uniform int chunkCount;

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
    uint index = gl_VertexID + drawOffsets[gl_InstanceID];
    Output results = outputs[index];
    uint x = index % gridSize;
    uint y = index / gridSize;
    uint chunkX = x / CHUNK;
    uint chunkY = y / CHUNK;

    int chunkIndex = int(chunkX * 2 + chunkY * chunkCount * 2);
    int isVisible = chunkPoses[chunkIndex];
    int lod = chunkPoses[chunkIndex + 1];
    visible = float(isVisible);

    // Pozice ve světovém prostoru
    vec4 worldPosition = model * vec4(results.position.x, results.position.y,results.position.z, 1.0);
    // Výpočet finální normály
    vec3 normal = normalize(mat3(transpose(inverse(model))) * results.normal.xyz);

    TexCoords = results.position.xz * 0.1; // Opakování textury každých 10 jednotek

    //Předání biomeID
    biomeID1 = results.biomeIDs[0];
    biomeID2 = results.biomeIDs[1];
    biomeID3 = results.biomeIDs[2];

    // Předání do fragment shaderu
    FragPos = worldPosition.xyz;
    Normal = normal;
    Weights = vec3(results.biomeWeight[0],results.biomeWeight[1],results.biomeWeight[2]);
    gl_Position = projection * view * worldPosition;
}
