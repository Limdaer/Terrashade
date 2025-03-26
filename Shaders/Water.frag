#version 460 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2DArray waterTextureArray;
uniform float waterFrame;

// Svetelne parametry
uniform vec3 lightDir;
uniform vec3 viewPos;
uniform vec3 fragWorldPos;
uniform float lightIntensity;
uniform float ambientStrength;
uniform float diffuseStrength;
uniform float specularStrength;

void main() {
    // Získání normály z normal mapy vody
    vec2 uv = TexCoords / 4.0;
    vec3 waterNormal = normalize(texture(waterTextureArray, vec3(uv, waterFrame)).rgb * 2.0 - 1.0);

    // Barva vody (základní)
    vec3 baseColor = vec3(0.0, 0.25, 0.8);

    // Konstanty pro světelné výpočty
    float shininess = 64.0;

    vec3 norm = normalize(waterNormal);
    vec3 lightDirNorm = normalize(-lightDir);

    // Ambient složka
    vec3 ambient = ambientStrength * baseColor * lightIntensity;

    // Diffuse složka
    float diff = max(dot(norm, lightDirNorm), 0.0);
    vec3 diffuse = diffuseStrength * diff * baseColor * lightIntensity;

    // Specular složka
    vec3 viewDir = normalize(viewPos - fragWorldPos);
    vec3 reflectDir = reflect(-lightDirNorm, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * vec3(1.0) * lightIntensity * 1.5;

    // Výsledná barva vody
    vec3 finalColor = ambient + diffuse + specular;

    FragColor = vec4(finalColor, 0.7); // 0.7 = průhlednost vody
}
