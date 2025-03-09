#version 460 core

out vec4 FragColor;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords; // Přijaté UV souřadnice

// Uniformy pro osvětlení
uniform vec3 lightDir = normalize(vec3(-1.0, -1.0, -1.0)); // Směr světla
uniform vec3 lightColor = vec3(1.0, 1.0, 0.9);  // Barva světla

// Parametry Phongova osvětlení
uniform float ambientStrength = 0.2;
uniform float diffuseStrength = 0.8;
uniform float specularStrength = 0.5;
uniform float shininess = 32.0;


// Kamera (pro spekulární světlo)
uniform vec3 viewPos;

// Textura terénu
uniform sampler2D terrainTexture;
uniform sampler2D normalMap;
uniform sampler2D roughnessMap;
uniform sampler2D aoMap;


vec3 GetNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, TexCoords).rgb;
    tangentNormal = tangentNormal * 2.0 - 1.0; // Konverze z [0,1] na [-1,1]
    return -normalize(tangentNormal);
}


void main() {
    // Normalizace normály
    vec3 norm = normalize(Normal); // Použití normálové textury místo interpolované normály

    float roughness = texture(roughnessMap, TexCoords).r;

    // AMBIENTNÍ SVĚTLO
    float ao = texture(aoMap, TexCoords).r;
    vec3 ambient = ambientStrength * lightColor * ao;

    // DIFUZNÍ SVĚTLO
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // SPEKULÁRNÍ SVĚTLO
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mix(4.0, 64.0, 1.0 - roughness)); 
    vec3 specular = specularStrength * spec * lightColor;


    // Načtení textury trávy
    vec3 textureColor = texture(terrainTexture, TexCoords).rgb;

    // Finální barva: kombinace textury a Phongova osvětlení
    vec3 result = (ambient + diffuse + specular) * textureColor;
    FragColor = vec4(result, 1.0);
}
