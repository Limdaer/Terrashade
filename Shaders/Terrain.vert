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

void main() {
    Output results = outputs[gl_VertexID];
    uint x = gl_VertexID % gridSize;
    uint y = gl_VertexID / gridSize;
    uint chunkX = x / CHUNK;
    uint chunkY = y / CHUNK;
    visible = float(chunkPoses[chunkX * 2 + chunkY * chunkCount * 2]);
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
