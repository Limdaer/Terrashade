#version 460 core

out vec4 FragColor;

in vec3 FragPos;  
in vec3 Normal;  
in vec2 TexCoords; // Přijaté UV souřadnice
flat in uint biomeID1;
flat in uint biomeID2;
flat in uint biomeID3;
in vec3 Weights;
in float visible;
flat in int lod;

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
uniform sampler2DArray waterTextureArray;
uniform float waterFrame;


vec3 CalculateLighting(
    sampler2D textureMap, sampler2D normalMap, sampler2D roughnessMap, sampler2D aoMap
) {
    vec2 texcoords = TexCoords;
    if (lod == 1) {
        texcoords *= 0.5;
    }
    else if (lod == 2) {
        texcoords *= 0.25;
    }
    else if (lod == 3) {
        texcoords *= 0.1;
    }

    // Načtení dat z textur
    vec3 textureColor = texture(textureMap, texcoords).rgb;
    vec3 normalTex = texture(normalMap, texcoords).rgb * 2.0 - 1.0;
    float roughness = texture(roughnessMap, texcoords).r;
    float ao = texture(aoMap, texcoords).r;

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


vec3 GetWaterColor() {
    vec2 texCoords = TexCoords / 4.0; // Zvětšení dlaždicování vody
    vec3 waterNormal = texture(waterTextureArray, vec3(texCoords, waterFrame)).rgb * 2.0 - 1.0;

    // Barva vody (modifikovaná podle normálové mapy)
    vec3 baseWaterColor = vec3(0.0, 0.2, 0.3);
    vec3 finalWaterColor = baseWaterColor + 0.1 * waterNormal;

    return finalWaterColor;
}

vec3 GetBiomeTexture(uint biomeID) {
    if (biomeID == 0) {
        return GetWaterColor();
    }
    else if (biomeID == 1) {
        return CalculateLighting(grassTexture, grassNormal, grassRough, grassAo);
    }
    else if (biomeID == 2) {
        return GetTextureByHeight();
    }
    else if (biomeID == 3) {
        return CalculateLighting(sandTexture, sandNormal, sandRough, sandAo);
    }
    return vec3(1.0, 0.0, 1.0);
}


void main() {
    float height = FragPos.y;

    vec3 col1 = GetBiomeTexture(biomeID1);
    vec3 col2 = GetBiomeTexture(biomeID2);
    vec3 col3 = GetBiomeTexture(biomeID3);

    vec3 finalColor;

    //Specialni blend pro more
    float seaFactor = 1.0 - smoothstep(1.0, 3.0, height);

    vec3 adjustedWeights = Weights;
    if (biomeID1 == 0) adjustedWeights.x *= seaFactor;
    if (biomeID2 == 0) adjustedWeights.y *= seaFactor;
    if (biomeID3 == 0) adjustedWeights.z *= seaFactor;

    float totalWeight = adjustedWeights.x + adjustedWeights.y + adjustedWeights.z;
    adjustedWeights /= totalWeight;

    finalColor = col1 * adjustedWeights.x + col2 * adjustedWeights.y + col3 * adjustedWeights.z;

    FragColor = vec4(finalColor, 1.0);
}
