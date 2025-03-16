#pragma once

#include <glad/glad.h>
#include <string>
#include <vector>

class Texture {
public:
    GLuint ID;
    Texture(const std::string& path);
    Texture(const std::vector<std::string>& faces);
    void Bind(GLuint unit = 0) const;
};
