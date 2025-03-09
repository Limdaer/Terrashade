#pragma once

#include <glad/glad.h>
#include <string>

class Texture {
public:
    GLuint ID;
    Texture(const std::string& path);
    void Bind(GLuint unit = 0) const;
};
