#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include <filesystem>
#include <vector>

class Texture {
public:
    GLuint ID;
    Texture(const std::string& path);
    Texture(const std::vector<std::string>& faces);
    Texture(const std::vector<std::string>& images, bool isArray);
    void Bind(GLuint unit = 0) const;
};
