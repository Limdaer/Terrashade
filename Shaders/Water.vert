#version 460 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoords;

void main() {
    TexCoords = aPos.xz * 0.1; // dlaždicování textury
    vec4 worldPos = model * vec4(aPos.x, -1.0, aPos.z, 1.0); // y = 0
    gl_Position = projection * view * worldPos;
}
