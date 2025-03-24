#include "Terrain.h"
#include <iostream>
#define BRUSHPREC (1024 * 16)
#define CHUNK 32


Terrain::Terrain(int gridSize, float worldSize) : gridSize(gridSize), worldSize(worldSize), 
computeShader("Shaders/Terrain.comp"), erosionShader("Shaders/Erosion.comp"), normalShader("Shaders/Normals.comp"), 
erosionApplyShader("Shaders/ErosionApply.comp") {
    GenerateTerrain();
    ComputeTerrain();
}


Terrain::~Terrain() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &resultsSSBO);

}

void Terrain::GenerateTerrain() {
    int chunksNum = (gridSize + CHUNK - 1) / CHUNK;
    // SSBO pro pozice (x, y, z)
    glGenBuffers(1, &resultsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize * sizeof(Output), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &intsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, intsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize * sizeof(int), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenBuffers(1, &chunkPosSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, chunkPosSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, (chunksNum * chunksNum * 2 * sizeof(int)), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Uniformbuffer
    glGenBuffers(1, &uniformBuffer);
    glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Uniforms), NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    // EBO pro indexy trojúhelníků
    std::vector<unsigned int> indices;
    for (int row = 0; row < gridSize - 1; row++) {
        for (int col = 0; col < gridSize - 1; col++) {
            int topLeft = row * gridSize + col;
            int topRight = topLeft + 1;
            int bottomLeft = (row + 1) * gridSize + col;
            int bottomRight = bottomLeft + 1;

            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);

            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }

    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}


void Terrain::Draw(Shader terrain,glm::vec3 cameraPos, float FOV, glm::vec3 viewDir) {
    glBindVertexArray(VAO);

    int chunksNum = (gridSize + CHUNK - 1) / CHUNK;
    float dx = gridSize / worldSize;
    glm::vec2 center;
    glm::vec2 v = glm::vec2(viewDir.x, viewDir.z);
    glm::vec2 perp = glm::vec2(-v.y, v.x);
    float sinAlpha = sinf(FOV + 1e-6f);

    // výškový faktor – čím výš jsi, tím víc rozšiřujeme výseč
    float heightFactor = cameraPos.y * 0.15;

    // fov faktor – čím větší fov, tím větší faktor (v radiánech)
    float fovFactor = glm::clamp(FOV / glm::radians(60.0f), 1.0f, 2.0f);

    // konečný rozšířený sinAlpha
    sinAlpha *= heightFactor * fovFactor;

    glm::vec2 v1 = glm::normalize(v) + sinAlpha * glm::normalize(perp);
    glm::vec2 v2 = glm::normalize(v) - sinAlpha * glm::normalize(perp);
    glm::vec2 v3 = glm::normalize(perp);
    glm::vec2 v1n = -glm::normalize(glm::normalize(perp) + sinAlpha * glm::normalize(v));
    glm::vec2 v2n = glm::normalize(glm::normalize(perp) - sinAlpha * glm::normalize(v));
    glm::vec2 v3n = -glm::normalize(v);
    float chunkR = sqrtf(2)* CHUNK / 2 * dx + cameraPos.y;

    chunksToRender.clear();

    for (int y = 0; y < chunksNum; y++) {
        for (int x = 0; x < chunksNum; x++) {
            float worldX = (x + 0.5f) * CHUNK - float(gridSize) / 2.0f * dx;
            float worldZ = (y + 0.5f) * CHUNK - float(gridSize) / 2.0f * dx;
            center = glm::vec2(worldX, worldZ);
            glm::vec2 relCenter = center - glm::vec2(cameraPos.x, cameraPos.z);

            float d1 = glm::dot(relCenter, v1n);
            float d2 = glm::dot(relCenter, v2n);
            float d3 = glm::dot(relCenter, v3n);
            if (d1 <= chunkR && d2 <= chunkR && d3 <= chunkR) {
                chunksToRender.push_back(1);
                chunksToRender.push_back(1);
            }
            else {
                chunksToRender.push_back(0);
                chunksToRender.push_back(0);
            }
        }
    }
    glNamedBufferSubData(chunkPosSSBO, 0, chunksToRender.size() * sizeof(int), chunksToRender.data());
    glUniform1i(glGetUniformLocation(terrain.ID, "chunkCount"), chunksNum);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, chunkPosSSBO);

    // Připojení SSBO jako zdroje dat pro VAO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, (gridSize - 1) * (gridSize - 1) * 6, GL_UNSIGNED_INT, 0);
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
    int dropletsPerPos = (erosion.numDroplets + gridSize * gridSize - 1) / (gridSize * gridSize);
    dropletIdx++;
    float brush[] = {
      0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f,
      0.0f, 0.2f, 0.4f, 0.6f, 0.7f, 0.6f, 0.4f, 0.2f, 0.0f,
      0.1f, 0.4f, 0.7f, 0.9f, 1.0f, 0.9f, 0.7f, 0.4f, 0.1f,
      0.2f, 0.6f, 0.9f, 1.0f, 1.0f, 1.0f, 0.9f, 0.6f, 0.2f,
      0.3f, 0.7f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.7f, 0.3f,
      0.2f, 0.6f, 0.9f, 1.0f, 1.0f, 1.0f, 0.9f, 0.6f, 0.2f,
      0.1f, 0.4f, 0.7f, 0.9f, 1.0f, 0.9f, 0.7f, 0.4f, 0.1f,
      0.0f, 0.2f, 0.4f, 0.6f, 0.7f, 0.6f, 0.4f, 0.2f, 0.0f,
      0.0f, 0.0f, 0.1f, 0.2f, 0.3f, 0.2f, 0.1f, 0.0f, 0.0f,
    };

    int brushInt[9 * 9];
    for (int i = 0; i < 9 * 9; i++) {
        brushInt[i] = int(brush[i] * BRUSHPREC);
    }
    
    
    // Nastavení uniformů
    glUniform1i(glGetUniformLocation(erosionShader.ID, "gridSize"), gridSize);
    glUniform1i(glGetUniformLocation(erosionShader.ID, "dropletIdx"), dropletIdx);
    glUniform1i(glGetUniformLocation(erosionShader.ID, "dropletsPerPos"), dropletsPerPos);
    glUniform1iv(glGetUniformLocation(erosionShader.ID, "brush_weights"), 9*9, brushInt);
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

                heights[index] += strength * gaussian; // Aplikace změny

                // Aktualizace SSBO
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultsSSBO);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, index * sizeof(glm::vec4) + sizeof(float), sizeof(float), &heights[index]);
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

