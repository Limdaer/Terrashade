#ifndef SKYBOX_H
#define SKYBOX_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include "Shader.h"
#include "Texture.h"

class Skybox {
public:
    Skybox();
    void Draw(Shader& shader);

private:
    unsigned int skyboxVAO, skyboxVBO;
    unsigned int cubemapTexture;

    void SetupSkybox();
};

#endif
