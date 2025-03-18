#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <glad/glad.h>
#include <glm/glm.hpp>

// Třída pro načítání, kompilaci a použití OpenGL shaderů
class Shader {
public:
    unsigned int ID; // Identifikátor shader programu

    // Konstruktor načítá vertex a fragment shader ze souborů a sestaví program
    Shader(const char* vertexPath, const char* fragmentPath);
    Shader(const char* computePath);

    // Aktivuje shader program
    void Use();

    // Nastaví matici 4x4 v shaderu (např. model, view, projection matice)
    void SetMat4(const char* name, const float* value);
    //nastaví i jiné proměnné
    void SetVec3(const char* name, float x, float y, float z);
    void SetVec3(const char* name, const glm::vec3& value);
    void SetFloat(const char* name, float value);
    void SetInt(const char* name, int value);
    void SetUInt(const char* name, unsigned int value);


private:
    // Načte kód shaderu ze souboru
    std::string LoadShaderSource(const char* filePath);

    // Zkontroluje chyby při kompilaci a linkování shaderu
    void CheckCompileErrors(unsigned int shader, std::string type);
};

#endif