#define STB_IMAGE_IMPLEMENTATION
#include "Texture.h"
#include "stb_image.h"
#include <iostream>

bool ReadImageMakeCache(const char* filepath, std::vector<uint8_t>* dataptr, int* width, int* height, int* nrChannels);

Texture::Texture(const std::string& path) {
    // Načtení obrázku pomocí STB Image
    int width, height, nrChannels;
    std::vector<uint8_t> data;

    if (ReadImageMakeCache(path.c_str(), &data, &width, &height, &nrChannels)) {
        glGenTextures(1, &ID);
        glBindTexture(GL_TEXTURE_2D, ID);

        // Debug výpis: Ověření vytvoření textury
        std::cout << "Nacitam texturu: " << path << std::endl;

        // Nastavení filtrů textury
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

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
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data.data());

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

Texture::Texture(const std::vector<std::string>& images, bool isArray) {
    if (!isArray) return;

    int width, height, nrChannels;
    std::vector<uint8_t> data;

    
    for (size_t i = 0; i < images.size(); i++) {
        if (ReadImageMakeCache(images[i].c_str(), &data, &width, &height, &nrChannels)) {
            GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
            if (i == 0) {
                glGenTextures(1, &ID);
                glBindTexture(GL_TEXTURE_2D_ARRAY, ID);
                glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, format, width, height, images.size(), 0, format, GL_UNSIGNED_BYTE, nullptr);
            }
            glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, width, height, 1, format, GL_UNSIGNED_BYTE, data.data());
        }
        else {
            std::cerr << "Chyba: Nepodarilo se nacist texturu " << images[i] << std::endl;
        }
    }

    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
}


void Texture::Bind(GLuint unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, ID);
}

struct Header {
    uint32_t magic;
    int32_t width;
    int32_t height;
    int32_t nrChannels;
    int32_t pixelSize; //=1
};

bool read_entire_file(const char* path, std::vector<uint8_t>* into)
{
    FILE* f = fopen(path, "rb");
    bool success = false;
    if (f) {
        fseek(f, 0, SEEK_END);
        int size = ftell(f);
        fseek(f, 0, SEEK_SET);

        into->resize(size);
        success = fread(into->data(), 1, size, f) == size;
        fclose(f);
    }

    return success;
}

bool ReadCached(const char* filepath, std::vector<uint8_t>* dataptr, int* width, int* height, int* nrChannels) {
    std::vector<uint8_t> file;
    if(read_entire_file(filepath, &file) == false)
        std::cout << "Couldnt read file." << std::endl;

    Header header = { 0 };
    int size = sizeof(header) < file.size() ? sizeof(header) : file.size();
    memcpy(&header, file.data(), size);

    if (header.magic != 0x78457443 || header.width < 0 || header.height < 0 || header.nrChannels < 0 || header.pixelSize < 0)
        return false;
    int reqSize = header.width * header.height * header.nrChannels * header.pixelSize;
    int remainingSize = file.size() - size;
    if (remainingSize < reqSize)
        return false;

    dataptr->resize(reqSize);
    memcpy(dataptr->data(), file.data() + size, reqSize);

    *width = header.width;
    *height = header.height;
    *nrChannels = header.nrChannels;
    return true;
}
bool WriteCached(const char* filepath,void* data, int width, int height, int nrChannels) {
    std::ofstream t(filepath, std::ios::binary);

    Header header = { 0 };
    header.magic = 0x78457443;
    header.width = width;
    header.height = height;
    header.nrChannels = nrChannels;
    header.pixelSize = 1;

    t.write((char*)(void*)&header, sizeof(header));
    t.write((char*)data, width * height * nrChannels);
    return t.good();
}

bool ReadImageMakeCache(const char* filepath, std::vector<uint8_t>* dataptr, int* width, int* height, int* nrChannels) {
    std::filesystem::path original = filepath;

    //split relative path of the original into segments (in reverse order)
    std::filesystem::path chopped = original.relative_path();
    std::vector<std::string> segments;

    while (chopped.empty() == false) {
        segments.push_back(chopped.filename().string());
        chopped = chopped.parent_path();
    }

    //add the segments (in reverse since they were added in reverse)
    std::string reconstructed;
    for (size_t i = segments.size(); i-- > 0;) {
        reconstructed += segments[i];
        if (i != 0)
            reconstructed += '_';
    }
    reconstructed += ".cached";


    //make the cached path
    std::filesystem::path cached_path = "cached";
    cached_path /= reconstructed;
    std::filesystem::create_directories(cached_path.parent_path());

    std::cout << "Cached: " << cached_path.string().c_str() << std::endl;
    std::cout << "OG: " << filepath << std::endl;

    std::error_code errorCache;
    std::error_code errorOg;
    std::filesystem::file_time_type timeCache = std::filesystem::last_write_time(cached_path, errorCache);
    std::filesystem::file_time_type timeSource = std::filesystem::last_write_time(filepath, errorOg);
    if (errorOg.value() != 0) {
        std::cout << "Couldnt find Og file." << std::endl;
        return false;
    }
    bool success = false;
    if (errorCache.value() == 0 || timeCache > timeSource) {
        success = ReadCached(cached_path.string().c_str(), dataptr, width, height, nrChannels);
        std::cout << "Found cached version: " << success << std::endl;
    }
    if(!success){
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(filepath, width, height, nrChannels, 0);
        if(data){
            dataptr->resize(*width * *height * *nrChannels);
            memcpy(dataptr->data(), data, *width * *height * *nrChannels);
            if(WriteCached(cached_path.string().c_str(), data, *width, *height, *nrChannels) == false)
                std::cout << "Write cache failed" << std::endl;
            else {
                std::vector<uint8_t> Try;
                ReadCached(cached_path.string().c_str(), &Try, width, height, nrChannels);
                for (int i = 0; i < Try.size(); i++) {
                    if (Try[i] != (*dataptr)[i])
                        std::cout << "Failed on index: " << i << std::endl;
                }
            }
            free(data);
            success = true;
        }
        std::cout << "Couldnt find cached version, attempting to load OG: " << success << std::endl;
    }
    return success;
}