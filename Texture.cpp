#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
#include "stb_image.h"
#include <iostream>

Texture::Texture(const std::string& path) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Debug výpis: Ověření vytvoření textury
    std::cout << "Nacitam texturu: " << path << std::endl;

    // Nastavení filtrů textury
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Načtení obrázku pomocí STB Image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB;
        if (nrChannels == 1) format = GL_RED;
        else if (nrChannels == 3) format = GL_RGB;
        else if (nrChannels == 4) format = GL_RGBA;
        else {
            std::cerr << "Neznamy format textury: " << path << " (" << nrChannels << " kanalu)" << std::endl;
        }

        // Debug výpis: Ověření rozměrů textury
        std::cout << "Rozmery textury: " << width << "x" << height << " (" << nrChannels << " kanalu)" << std::endl;

        // Nastavení dat textury
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // Kontrola OpenGL chyb před generováním mipmap
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL chyba pred glGenerateMipmap: " << err << std::endl;
        }

        // Generování mipmap
        glGenerateMipmap(GL_TEXTURE_2D);

        // Kontrola OpenGL chyb po generování mipmap
        err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL chyba po glGenerateMipmap : " << err << std::endl;
        }
    }
    else {
        std::cerr << "Chyba: Nepodarilo se nacist texturu " << path << std::endl;
    }

    stbi_image_free(data);
}

Texture::Texture(const std::vector<std::string>& faces) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

    stbi_set_flip_vertically_on_load(false);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "Chyba: Nepodarilo se nacist cubemap texturu: " << faces[i] << std::endl;
        }
    }

    // Nastavení parametrů pro skybox
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Texture::Bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}
