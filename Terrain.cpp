#include "Terrain.h"
#include <iostream>
#define PRECISION (1024 * 16)
#define CHUNK 33
#define CHUNK_FACES 32


Terrain::Terrain(int gridSize, float worldSize) : worldSize(worldSize),
computeShader("Shaders/Terrain.comp"), erosionShader("Shaders/Erosion.comp"), normalShader("Shaders/Normals.comp"),
erosionApplyShader("Shaders/ErosionApply.comp") {
    this->gridSize = (gridSize + CHUNK - 1) / CHUNK * CHUNK;
    GenerateTerrain();
    ComputeTerrain();
}


Terrain::~Terrain() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBOLOD1);
    glDeleteBuffers(1, &EBOLOD2);
    glDeleteBuffers(1, &EBOLOD4);
    glDeleteBuffers(1, &resultsSSBO);
    glDeleteBuffers(1, &intsSSBO);
    glDeleteBuffers(1, &uniformBuffer);
    glDeleteBuffers(1, &chunkPosSSBO);
}

std::vector<unsigned int> GenerateTerrainIdxBuffer(int rows, int cols, int gridSize, int lodLevel) {
    std::vector<unsigned int> indices;
    for (int row = 0; row < rows - lodLevel; row += lodLevel) {
        for (int col = 0; col < cols - lodLevel; col += lodLevel) {
            int topLeft = row * gridSize + col;
            int topRight = topLeft + lodLevel;
            int bottomLeft = (row + lodLevel) * gridSize + col;
            int bottomRight = bottomLeft + lodLevel;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    return indices;
}

void Terrain::GenerateTerrain() {
    int chunksNum = (gridSize + CHUNK - 1) / CHUNK;
    // SSBO pro pozice (x, y, z)
    glGenBuffers(1, &resultsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize
        * sizeof(Output), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &intsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize * sizeof(int), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &chunkPosSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkPosSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (chunksNum * chunksNum * sizeof(int)), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &drawOffsetSSBO1);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawOffsetSSBO1);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (chunksNum * chunksNum * sizeof(ChunkDraw)), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &drawOffsetSSBO2);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawOffsetSSBO2);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (chunksNum * chunksNum * sizeof(ChunkDraw)), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &drawOffsetSSBO4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, drawOffsetSSBO4);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (chunksNum * chunksNum * sizeof(ChunkDraw)), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Uniformbuffer
    glGenBuffers(1, &uniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniforms), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // EBO pro indexy trojúhelníků
    std::vector<unsigned int> indices1 = GenerateTerrainIdxBuffer(CHUNK, CHUNK, gridSize, 1);
    std::vector<unsigned int> indices2 = GenerateTerrainIdxBuffer(CHUNK, CHUNK, gridSize, 2);
    std::vector<unsigned int> indices4 = GenerateTerrainIdxBuffer(CHUNK, CHUNK, gridSize, 4);



    glGenBuffers(1, &EBOLOD1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOLOD1);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices1.size() * sizeof(unsigned int), indices1.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBOLOD2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOLOD2);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices2.size() * sizeof(unsigned int), indices2.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &EBOLOD4);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOLOD4);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices4.size() * sizeof(unsigned int), indices4.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

glm::vec4 createPlane(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 normal = glm::normalize(glm::cross(a - b, c - b));
    float dist = -glm::dot(normal, a);
    return glm::vec4(normal, dist);
}

glm::vec3 worldDiv(glm::vec4 a) {
    return glm::vec3(a / a.w);
}

bool isInFrustum(glm::vec3 minCorner, glm::vec3 maxCorner, glm::vec4 planes[6], float chunkWorldSize) {
    glm::vec3 extent = (maxCorner - minCorner) * 0.5f;
    glm::vec3 center = (minCorner + maxCorner) * 0.5f;
    float treshold = -chunkWorldSize;
    for (int i = 0; i < 6; i++) {
        glm::vec3 normal = glm::vec3(planes[i]);
        float dist = planes[i].w;
        float radius = extent.x * std::abs(normal.x) +
            extent.y * std::abs(normal.y) +
            extent.z * std::abs(normal.z);
        float d = glm::dot(normal, center) + dist;
        if (d + radius < treshold)
            return false;
    }
    return true;
}

void Terrain::Draw(Shader terrain, glm::mat4 view, glm::mat4 projection, glm::vec3 cameraPos) {
    //glm::mat4 cullView = glm::identity<glm::mat4>();
    //glm::mat4 cullProjection = glm::perspective(glm::radians(90.0f), 800.0f / 600.0f, 1.0f, 10.0f);

    glm::mat4 invViewProjection = glm::inverse(projection * view);
    glm::vec4 ndc_near00 = glm::vec4(-1, -1, -1, 1);
    glm::vec4 ndc_near01 = glm::vec4(-1, 1, -1, 1);
    glm::vec4 ndc_near10 = glm::vec4(1, -1, -1, 1);
    glm::vec4 ndc_near11 = glm::vec4(1, 1, -1, 1);
    glm::vec4 ndc_far00 = glm::vec4(-1, -1, 1, 1);
    glm::vec4 ndc_far01 = glm::vec4(-1, 1, 1, 1);
    glm::vec4 ndc_far10 = glm::vec4(1, -1, 1, 1);
    glm::vec4 ndc_far11 = glm::vec4(1, 1, 1, 1);

    glm::vec3 world_near00 = worldDiv(invViewProjection * ndc_near00);
    glm::vec3 world_near01 = worldDiv(invViewProjection * ndc_near01);
    glm::vec3 world_near10 = worldDiv(invViewProjection * ndc_near10);
    glm::vec3 world_near11 = worldDiv(invViewProjection * ndc_near11);
    glm::vec3 world_far00 = worldDiv(invViewProjection * ndc_far00);
    glm::vec3 world_far01 = worldDiv(invViewProjection * ndc_far01);
    glm::vec3 world_far10 = worldDiv(invViewProjection * ndc_far10);
    glm::vec3 world_far11 = worldDiv(invViewProjection * ndc_far11);

    glm::vec4 planes[6];

    planes[0] = createPlane(world_near11, world_near10, world_far10);
    planes[1] = createPlane(world_near01, world_near11, world_far11);
    planes[2] = createPlane(world_near00, world_near01, world_far01);
    planes[3] = createPlane(world_near10, world_near00, world_far00);
    planes[4] = createPlane(world_near00, world_near10, world_near11);
    planes[5] = createPlane(world_far11, world_far10, world_far00);
    glBindVertexArray(VAO);
    int chunksNum = (gridSize + CHUNK - 1) / CHUNK;
    float dx = gridSize / worldSize;


    chunksToRender.clear();
    drawOffsets1.clear();
    drawOffsets2.clear();
    drawOffsets4.clear();

    for (int y = 0; y < chunksNum; y++) {
        for (int x = 0; x < chunksNum; x++) {
            float minX = x * CHUNK * dx - (gridSize * 0.5f) * dx;
            float minY = -100;
            float minZ = y * CHUNK * dx - (gridSize * 0.5f) * dx;
            float maxX = (x + 1) * CHUNK * dx - (gridSize * 0.5f) * dx;
            float maxY = 100;
            float maxZ = (y + 1) * CHUNK * dx - (gridSize * 0.5f) * dx;
            glm::vec2 center = glm::vec2((minX + maxX) / 2, (minZ + maxZ) / 2);
            bool isIn = isInFrustum(glm::vec3(minX, minY, minZ), glm::vec3(maxX, maxY, maxZ), planes, CHUNK * dx);
            //bool isIn = true;
            int chunkOffset = (y * gridSize + x) * CHUNK_FACES;
            ChunkDraw chunkDraw = { 0 };
            chunkDraw.vertexOffset = chunkOffset;
            chunkDraw.chunkX = x;
            chunkDraw.chunkY = y;
            if (isIn) {
                // Vzdálenost pro LOD
                float dist = glm::length(center - glm::vec2(cameraPos.x, cameraPos.z));
                //float dist = 20;
                int lod = 0;
                if (dist > 400.0f) {
                    lod = 4;
                    drawOffsets4.push_back(chunkDraw);
                }
                else if (dist > 200.0f) {
                    lod = 2;
                    drawOffsets2.push_back(chunkDraw);
                }
                else {
                    lod = 1;
                    drawOffsets1.push_back(chunkDraw);
                }

                chunksToRender.push_back(lod);    // LOD úroveň
            }
            else {
                chunksToRender.push_back(0);      // Nezáleží
            }
        }
    }

    terrain.Use();
    glNamedBufferSubData(chunkPosSSBO, 0, chunksToRender.size() * sizeof(int), chunksToRender.data());
    glUniform1i(glGetUniformLocation(terrain.ID, "chunkCount"), chunksNum);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkPosSSBO);

    glNamedBufferSubData(drawOffsetSSBO1, 0, drawOffsets1.size() * sizeof(ChunkDraw), drawOffsets1.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawOffsetSSBO1);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOLOD1);
    glDrawElementsInstanced(GL_TRIANGLES, CHUNK_FACES * CHUNK_FACES * 6, GL_UNSIGNED_INT, 0, drawOffsets1.size());

    glNamedBufferSubData(drawOffsetSSBO2, 0, drawOffsets2.size() * sizeof(ChunkDraw), drawOffsets2.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawOffsetSSBO2);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOLOD2);
    glDrawElementsInstanced(GL_TRIANGLES, CHUNK_FACES * CHUNK_FACES / 4 * 6, GL_UNSIGNED_INT, 0, drawOffsets2.size());

    glNamedBufferSubData(drawOffsetSSBO4, 0, drawOffsets4.size() * sizeof(ChunkDraw), drawOffsets4.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, drawOffsetSSBO4);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBOLOD4);
    glDrawElementsInstanced(GL_TRIANGLES, CHUNK_FACES * CHUNK_FACES / 16 * 6, GL_UNSIGNED_INT, 0, drawOffsets4.size());
}


void Terrain::ComputeTerrain() {
    computeShader.Use();
    uniforms.Dunes = Params{ 0.0, 0.0,  // fbm
            0.0, 0.0,  // ridge
            0.0, 0.0,  // voronoi
            0.0, 0.0,  // morphed voronoi
            2.0, 0.5 }; // sand dunes                                 

    uniforms.Plains = Params{ 0.4, 0.5,  // fbm
            0.0, 0.0,  // ridge
            0.0, 0.0,  // voronoi
            0.0, 0.0,  // morphed voronoi
            0.0, 0.0 }; // sand dunes

    uniforms.Mountains = Params{ 0.0, 0.0,  // fbm
            2.0, 0.8,  // ridge
            0.8, 2.0,  // voronoi
            2.0, 2.0,  // morphed voronoi
            0.0, 0.0 }; // sand dunes

    uniforms.Sea = Params{ 0.0, 0.0,  // fbm
        0.0, 0.0,
        0.0, 0.0,
        0.0, 0.0,
        0.0, 0.0 };

    glNamedBufferSubData(uniformBuffer, 0, sizeof(uniforms), &uniforms);
    glUniform1i(glGetUniformLocation(computeShader.ID, "gridSize"), gridSize);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, uniformBuffer);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1); // Vypocty
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Zápis do bufferu před čtením

    glUseProgram(0);
}

void Terrain::ComputeNormals() {
    normalShader.Use();

    glUniform1i(glGetUniformLocation(normalShader.ID, "gridSize"), gridSize);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);
}
//Eroze
void Terrain::ComputeErosion(Erosion erosion) {
    erosionShader.Use(); // Aktivace erosion compute shaderu


    // Nastavení uniformů
    glUniform1i(glGetUniformLocation(erosionShader.ID, "gridSize"), gridSize);
    glUniform1i(glGetUniformLocation(erosionShader.ID, "dropletIdx"), dropletIdx);
    glUniform1i(glGetUniformLocation(erosionShader.ID, "numDroplets"), erosion.numDroplets); // Počet kapek vody
    glUniform1f(glGetUniformLocation(erosionShader.ID, "erosionRate"), erosion.erosionRate);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "depositionRate"), erosion.depositionRate);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "inertia"), erosion.inertia);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "sedimentCapacityFactor"), erosion.sedimentCapacityFactor);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "minSedimentCapacity"), erosion.minSedimentCapacity);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "erodeSpeed"), erosion.erodeSpeed);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "depositSpeed"), erosion.depositSpeed);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "gravity"), erosion.gravity);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "initialWaterVolume"), erosion.initialWaterVolume);
    glUniform1f(glGetUniformLocation(erosionShader.ID, "initialSpeed"), erosion.initialSpeed);

    // Připojení SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);
    //Vysledek eroze
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, intsSSBO);

    // Spuštění výpočtu compute shaderu
    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Synchronizace s GPU

    erosionApplyShader.Use();
    glUniform1i(glGetUniformLocation(erosionApplyShader.ID, "gridSize"), gridSize);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);
}

void Terrain::UpdateTerrain(float scale, float edgeSharpness, float heightScale, int octaves,
    float persistence, float lacunarity, unsigned int seed) {
    computeShader.Use();  // Aktivace compute shaderu

    computeShader.SetFloat("scale", scale);
    computeShader.SetFloat("edgeSharpness", edgeSharpness);
    computeShader.SetFloat("heightScale", heightScale);
    computeShader.SetUInt("octaves", octaves);
    computeShader.SetFloat("persistence", persistence);
    computeShader.SetFloat("lacunarity", lacunarity);
    computeShader.SetUInt("seed", seed);
    computeShader.SetFloat("gridDx", worldSize / gridSize);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Terrain::ReadHeightsFromSSBO() {
    if (resultsSSBO == 0) {
        std::cerr << "Chyba: SSBO neni inicializovano!\n";
        return;
    }

    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_BUFFER_UPDATE_BARRIER_BIT);

    std::vector<Output> tempData(gridSize * gridSize);
    heights.resize(gridSize * gridSize);
    biomeIDs.resize(gridSize * gridSize * 3);  // 3 ID na každý bod
    biomeWeights.resize(gridSize * gridSize * 3); // 3 váhy na každý bod

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultsSSBO);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, tempData.size() * sizeof(Output), tempData.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Uložení dat do vektorů
    for (size_t i = 0; i < tempData.size(); ++i) {
        heights[i] = tempData[i].position.y;

        biomeIDs[i * 3 + 0] = tempData[i].biomeIDs[0];
        biomeIDs[i * 3 + 1] = tempData[i].biomeIDs[1];
        biomeIDs[i * 3 + 2] = tempData[i].biomeIDs[2];

        biomeWeights[i * 3 + 0] = tempData[i].biomeWeight[0];
        biomeWeights[i * 3 + 1] = tempData[i].biomeWeight[1];
        biomeWeights[i * 3 + 2] = tempData[i].biomeWeight[2];
    }
}

void Terrain::DrawWater(Shader& waterShader, float currentFrame, const glm::mat4& view, const glm::mat4& projection) {
    static unsigned int waterVAO = 0, waterVBO = 0, waterEBO = 0;

    if (waterVAO == 0) {
        float size = worldSize;
        float vertices[] = {
            -size / 2, 0.0f, -size / 2,
             size / 2, 0.0f, -size / 2,
             size / 2, 0.0f,  size / 2,
            -size / 2, 0.0f,  size / 2
        };
        unsigned int indices[] = {
            0, 2, 1,
            0, 3, 2
        };


        glGenVertexArrays(1, &waterVAO);
        glGenBuffers(1, &waterVBO);
        glGenBuffers(1, &waterEBO);

        glBindVertexArray(waterVAO);
        glBindBuffer(GL_ARRAY_BUFFER, waterVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, waterEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    // Aktivace shaderu a nastavení matic
    waterShader.Use();
    glm::mat4 model = glm::mat4(1.0f);
    waterShader.SetMat4("model", glm::value_ptr(model));
    waterShader.SetMat4("view", glm::value_ptr(view));
    waterShader.SetMat4("projection", glm::value_ptr(projection));
    waterShader.SetFloat("waterFrame", currentFrame);

    // Voda má průhlednost
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(waterVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}



float Terrain::GetHeightAt(float worldX, float worldZ) {
    // Převod světových souřadnic na indexy v mřížce
    int x = static_cast<int>(worldX + gridSize / 2);
    int z = static_cast<int>(worldZ + gridSize / 2);

    // Ověření, že indexy jsou v platném rozsahu
    if (x < 0 || x >= gridSize || z < 0 || z >= gridSize) {
        return 0.0f; // Vrátíme výchozí hodnotu, pokud je mimo rozsah
    }

    // Výpočet indexu v 1D poli
    int index = z * gridSize + x;

    // Vrácení odpovídající výšky
    return heights[index];
}

//Zmena terenu pomoci gaussovy krivky
void Terrain::ModifyTerrain(glm::vec3 hitPoint, int mode) {
    int centerX = static_cast<int>(hitPoint.x + gridSize / 2);
    int centerZ = static_cast<int>(hitPoint.z + gridSize / 2);


    for (int dz = -radius; dz <= radius; dz++) {
        for (int dx = -radius; dx <= radius; dx++) {
            int x = centerX + dx;
            int z = centerZ + dz;

            if (x >= 0 && x < gridSize && z >= 0 && z < gridSize) {
                float distance = sqrt(dx * dx + dz * dz);
                if (distance > radius) continue; // Omezíme působnost na kruh

                float gaussian = exp(-(distance * distance) / (2 * sigma * sigma));
                int index = z * gridSize + x;

                heights[index] += strength * gaussian * mode; // Aplikace změny

                // Aktualizace SSBO
                Output output;
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultsSSBO);
                glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, index * sizeof(Output), sizeof(Output), &output);

                // Úprava výšky
                output.position.y = heights[index];

                // Zápis celé struktury zpět
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, index * sizeof(Output), sizeof(Output), &output);
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

            }
        }
    }
}

void Terrain::UpdateBiomeParams(const Params& dunes, const Params& plains, const Params& mountains, const Params& sea) {
    computeShader.Use();

    uniforms.Dunes = dunes;

    uniforms.Plains = plains;

    uniforms.Mountains = mountains;

    uniforms.Sea = sea;

    glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(uniforms), &uniforms);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void Terrain::SaveHeightmapAsPNG(const std::string& filename) {
    if (heights.empty()) {
        std::cerr << "Chyba: Heightmapa neni nactena!\n";
        return;
    }

    std::vector<uint8_t> imageData(gridSize * gridSize);

    float minH = *std::min_element(heights.begin(), heights.end());
    float maxH = *std::max_element(heights.begin(), heights.end());
    float heightRange = maxH - minH;
    if (heightRange == 0.0f) heightRange = 1.0f;

    for (size_t i = 0; i < heights.size(); ++i) {
        imageData[i] = static_cast<uint8_t>(255.0f * (heights[i] - minH) / (maxH - minH));
    }

    stbi_write_png(filename.c_str(), gridSize, gridSize, 1, imageData.data(), gridSize);

    std::cout << "Heightmapa ulozena jako " << filename << std::endl;
}

void Terrain::SaveBiomeIDsAsPNG(const std::string& filename) {
    if (biomeIDs.empty() || gridSize == 0) {
        std::cerr << "Chyba: Biome ID data nejsou nactena!\n";
        return;
    }

    std::vector<uint8_t> imageData(gridSize * gridSize * 3);

    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        imageData[i * 3 + 0] = static_cast<uint8_t>(biomeIDs[i * 3 + 0] * 64);
        imageData[i * 3 + 1] = static_cast<uint8_t>(biomeIDs[i * 3 + 1] * 64);
        imageData[i * 3 + 2] = static_cast<uint8_t>(biomeIDs[i * 3 + 2] * 64);
    }

    if (!stbi_write_png(filename.c_str(), gridSize, gridSize, 3, imageData.data(), gridSize * 3)) {
        std::cerr << "Chyba: Ukladani biome ID mapy selhalo!\n";
    }
    else {
        std::cout << "Biome ID mapa ulozena jako " << filename << std::endl;
    }
}

void Terrain::SaveBlendWeightsAsPNG(const std::string& filename) {
    if (biomeWeights.empty() || gridSize == 0) {
        std::cerr << "Chyba: Biome váhy nejsou nactene!\n";
        return;
    }

    std::vector<uint8_t> imageData(gridSize * gridSize * 3);

    for (size_t i = 0; i < gridSize * gridSize; ++i) {
        imageData[i * 3 + 0] = static_cast<uint8_t>(255.0f * biomeWeights[i * 3 + 0]);
        imageData[i * 3 + 1] = static_cast<uint8_t>(255.0f * biomeWeights[i * 3 + 1]);
        imageData[i * 3 + 2] = static_cast<uint8_t>(255.0f * biomeWeights[i * 3 + 2]);
    }

    if (!stbi_write_png(filename.c_str(), gridSize, gridSize, 3, imageData.data(), gridSize * 3)) {
        std::cerr << "Chyba: Ukladani biome weight mapy selhalo!\n";
    }
    else {
        std::cout << "Biome weight mapa ulozena jako " << filename << std::endl;
    }
}

