#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/gtc/type_ptr.hpp>

// Constructor loads, compiles, and links vertex and fragment shaders
Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::cout << "Loading shaders: " << vertexPath << " and " << fragmentPath << std::endl;

    // 1. Load shader source code from files
    std::string vertexCode = LoadShaderSource(vertexPath);
    std::string fragmentCode = LoadShaderSource(fragmentPath);
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Create shaders
    unsigned int vertex, fragment;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");

    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");

    // 3. Link shaders into a single program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");

    // 4. Delete unnecessary shader objects
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

Shader::Shader(const char* computePath) {
    std::cout << "Loading Compute Shader: " << computePath << std::endl;

    std::string computeCode = LoadShaderSource(computePath);
    const char* cShaderCode = computeCode.c_str();

    unsigned int compute;
    compute = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(compute, 1, &cShaderCode, NULL);
    glCompileShader(compute);
    CheckCompileErrors(compute, "COMPUTE");

    ID = glCreateProgram();
    glAttachShader(ID, compute);
    glLinkProgram(ID);
    CheckCompileErrors(ID, "PROGRAM");

    glDeleteShader(compute);

    std::cout << "Compute Shader Loaded Successfully!" << std::endl;
}

// Aktivuje shader
void Shader::Use() {
    glUseProgram(ID);
}

// Načíst glsl ze souboru
std::string Shader::LoadShaderSource(const char* filePath) {
    std::ifstream file;
    std::stringstream buffer;
    file.open(filePath);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open shader file " << filePath << std::endl;
        return "";
    }
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

// Zjistit errory
void Shader::CheckCompileErrors(unsigned int shader, std::string type) {
    int success;
    char infoLog[1024];
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR: Compilation of " << type << " shader failed!\n" << infoLog << std::endl;
        }
        else {
            std::cout << type << " shader successfully compiled." << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "ERROR: Shader program linking failed!\n" << infoLog << std::endl;
        }
        else {
            std::cout << "Shader program successfully linked." << std::endl;
        }
    }
}

// Nastaví matici 4x4 v shaderu
void Shader::SetMat4(const std::string& name, const float* value) {
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, value);
}

// Nastaví uniformní proměnnou vec3 (ruční zadání tří složek)
void Shader::SetVec3(const std::string& name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

// Nastaví uniformní proměnnou vec3 (přes glm::vec3)
void Shader::SetVec3(const std::string& name, const glm::vec3& value) {
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(value));
}

// Nastaví uniformní proměnnou float
void Shader::SetFloat(const std::string& name, float value) {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

// Nastaví uniformní proměnnou int
void Shader::SetInt(const std::string& name, int value) {
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::SetUInt(const std::string& name, unsigned int value) {
    glUniform1ui(glGetUniformLocation(ID, name.c_str()), value);
}