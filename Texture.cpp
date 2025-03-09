#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
#include "stb_image.h"
#include <iostream>

Texture::Texture(const std::string& path) {
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    // Debug výpis: Ověření vytvoření textury
    std::cout << "Načítám texturu: " << path << std::endl;

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
            std::cerr << "Neznámý formát textury: " << path << " (" << nrChannels << " kanálů)" << std::endl;
        }

        // Debug výpis: Ověření rozměrů textury
        std::cout << "Rozměry textury: " << width << "x" << height << " (" << nrChannels << " kanálů)" << std::endl;

        // Nastavení dat textury
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

        // Kontrola OpenGL chyb před generováním mipmap
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cerr << "OpenGL chyba před glGenerateMipmap: " << err << std::endl;
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
        std::cerr << "Chyba: Nepodařilo se načíst texturu " << path << std::endl;
    }

    stbi_image_free(data);
}

void Texture::Bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}
