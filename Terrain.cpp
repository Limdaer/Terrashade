#include "Terrain.h"
#include <iostream>

Terrain::Terrain(int gridSize) : gridSize(gridSize), computeShader("Shaders/Terrain.comp"), erosionShader("Shaders/Erosion.comp") {
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

    // SSBO pro pozice (x, y, z)
    glGenBuffers(1, &resultsSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, resultsSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, gridSize * gridSize * sizeof(Output), NULL, GL_DYNAMIC_DRAW);
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


void Terrain::Draw() {
    glBindVertexArray(VAO);

    // Připojení SSBO jako zdroje dat pro VAO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,0 ,resultsSSBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glDrawElements(GL_TRIANGLES, (gridSize - 1) * (gridSize - 1) * 6,GL_UNSIGNED_INT, 0);
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
    computeShader.Use();

    glUniform1i(glGetUniformLocation(computeShader.ID, "computeNormalsOnly"), 1);

    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    glUseProgram(0);
}

void Terrain::ComputeErosion() {
    erosionShader.Use(); // Aktivace erosion compute shaderu

    // Nastavení uniformů
    glUniform1i(glGetUniformLocation(erosionShader.ID, "gridSize"), gridSize);
    glUniform1i(glGetUniformLocation(erosionShader.ID, "numDroplets"), 70000); // Počet kapek vody

    // Připojení SSBO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, resultsSSBO);

    // Spuštění výpočtu compute shaderu
    glDispatchCompute((gridSize + 15) / 16, (gridSize + 15) / 16, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT); // Synchronizace s GPU

    glUseProgram(0);
}






void Terrain::UpdateTerrain(float scale, float edgeSharpness, float heightScale, int octaves, float persistence, float lacunarity, unsigned int seed) {
    computeShader.Use();  // Aktivace compute shaderu

    computeShader.SetFloat("scale", scale);
    computeShader.SetFloat("edgeSharpness", edgeSharpness);
    computeShader.SetFloat("heightScale", heightScale);
    computeShader.SetUInt("octaves", octaves);
    computeShader.SetFloat("persistence", persistence);
    computeShader.SetFloat("lacunarity", lacunarity);
    computeShader.SetUInt("seed", seed);

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


