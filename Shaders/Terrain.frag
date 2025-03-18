#version 460 core

out vec4 FragColor;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords; // Přijaté UV souřadnice
flat in uint biomeID1;
flat in uint biomeID2;
flat in uint biomeID3;
in vec3 Weights;

// Uniformy pro osvětlení
uniform vec3 lightDir; // Směr světla
uniform vec3 lightColor;  // Barva světla
uniform float lightIntensity;

// Parametry Phongova osvětlení
uniform float ambientStrength;
uniform float diffuseStrength = 0.8;
uniform float specularStrength;
uniform float shininess = 32.0;


// Kamera (pro spekulární světlo)
uniform vec3 viewPos;

// Výška pro mixování textur
uniform float grassHeight = 10.0;
uniform float rockHeight = 15.0;
uniform float blendRange = 5.0;

// Textura terénu
uniform sampler2D grassTexture, grassNormal, grassRough, grassAo;
uniform sampler2D rockTexture, rockNormal, rockRough, rockAo;
uniform sampler2D snowTexture, snowNormal, snowRough, snowAo;
uniform sampler2D sandTexture, sandNormal, sandRough, sandAo;


vec3 CalculateLighting(
    sampler2D textureMap, sampler2D normalMap, sampler2D roughnessMap, sampler2D aoMap
) {
    // Načtení dat z textur
    vec3 textureColor = texture(textureMap, TexCoords).rgb;
    vec3 normalTex = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    float roughness = texture(roughnessMap, TexCoords).r;
    float ao = texture(aoMap, TexCoords).r;

    // Použití normály z SSBO místo normal mapy
    vec3 norm = normalize(Normal);

    // AMBIENTNÍ SVĚTLO
    vec3 ambient = ambientStrength * lightColor * ao * lightIntensity;

    // DIFUZNÍ SVĚTLO
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor * lightIntensity;

    // SPEKULÁRNÍ SVĚTLO
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor * lightIntensity;

    // Finální barva: kombinace textury a Phongova osvětlení
    return (ambient + diffuse + specular) * textureColor;
}

vec3 GetTextureByHeight() {
    float height = FragPos.y;

    vec3 grassColor = CalculateLighting(grassTexture, grassNormal, grassRough, grassAo);
    vec3 rockColor = CalculateLighting(rockTexture, rockNormal, rockRough, rockAo);
    vec3 snowColor = CalculateLighting(snowTexture, snowNormal, snowRough, snowAo);

    // Výpočet blendingu mezi texturami pomocí smoothstep
    float grassBlend = 1.0 - smoothstep(grassHeight, grassHeight + blendRange, height);
    float rockBlend = smoothstep(grassHeight, grassHeight + blendRange, height) * (1.0 - smoothstep(rockHeight, rockHeight + blendRange, height));
    float snowBlend = smoothstep(rockHeight, rockHeight + blendRange, height);

    // Lineární interpolace mezi texturami
    return grassColor * grassBlend + rockColor * rockBlend + snowColor * snowBlend;
}

vec3 GetBiomeTexture(uint biomeID) {
    if (biomeID == 0) {
        return CalculateLighting(sandTexture, sandNormal, sandRough, sandAo);
    }
    else if (biomeID == 1) {
        return CalculateLighting(grassTexture, grassNormal, grassRough, grassAo);
    }
    else if (biomeID == 2) {
        return GetTextureByHeight();
    }
    return vec3(1.0, 0.0, 1.0);
}


void main() {
    vec3 col1 = GetBiomeTexture(biomeID1);
    vec3 col2 = GetBiomeTexture(biomeID2);
    vec3 col3 = GetBiomeTexture(biomeID3);
    vec3 finalColor = col1 * Weights.x + col2 * Weights.y + col3 * Weights.z;

    FragColor = vec4(finalColor, 1.0);
}
