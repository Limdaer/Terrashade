#version 460 core

layout(location = 0) in vec3 aPos;    // Pozice z SSBO
layout(location = 1) in vec3 aNormal; // Normála z SSBO
layout(location = 2) in vec2 aTexCoord; // UV souřadnice

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;  
out vec3 Normal;
out vec2 TexCoords; // Výstup pro fragment shader

void main() {
    // Pozice ve světovém prostoru
    vec4 worldPosition = model * vec4(aPos, 1.0);
    
    // Výpočet finální normály
    vec3 normal = normalize(mat3(transpose(inverse(model))) * aNormal);
    
    // Generování UV souřadnic (pokud je nemáš v SSBO, použij derivaci XZ)
    TexCoords = aPos.xz * 0.1; // Opakování textury každých 10 jednotek

    // Předání do fragment shaderu
    FragPos = vec3(worldPosition);
    Normal = normal;

    gl_Position = projection * view * worldPosition;
}
