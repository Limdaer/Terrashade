#version 460 core

layout(location = 0) in vec3 aPos;  // Pozice z SSBO (uložené jako vec4, ale čteme jen vec3)
layout(location = 1) in vec3 aNormal; // Normála z SSBO (uložená jako vec4, ale čteme jen vec3)

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;  
out vec3 Normal;

void main() {
    // Pozice ve světovém prostoru
    vec4 worldPosition = model * vec4(aPos, 1.0);
    
    // Výpočet finální normály
    vec3 normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    // Předání do fragment shaderu
    FragPos = vec3(worldPosition);
    Normal = normal;

    gl_Position = projection * view * worldPosition;
}
