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

// Výška pro mixování textur
uniform float grassHeight = 10.0;
uniform float rockHeight = 25.0;
uniform float snowHeight = 40.0;

// Textura terénu
uniform sampler2D grassTexture, grassNormal, grassRough, grassAo;
uniform sampler2D rockTexture, rockNormal, rockRough, rockAo;
uniform sampler2D snowTexture, snowNormal, snowRough, snowAo;


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
    vec3 ambient = ambientStrength * lightColor * ao;

    // DIFUZNÍ SVĚTLO
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // SPEKULÁRNÍ SVĚTLO
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), mix(4.0, 64.0, 1.0 - roughness)); 
    vec3 specular = specularStrength * spec * lightColor;

    // Finální barva: kombinace textury a Phongova osvětlení
    return (ambient + diffuse + specular) * textureColor;
}

vec3 GetTextureByHeight() {
    float height = FragPos.y;

    if (height < grassHeight)
        return CalculateLighting(grassTexture, grassNormal, grassRough, grassAo);
    else if (height < rockHeight)
        return CalculateLighting(rockTexture, rockNormal, rockRough, rockAo);
    else
        return CalculateLighting(snowTexture, snowNormal, snowRough, snowAo);
}


void main() {
    vec3 finalColor = GetTextureByHeight();

    FragColor = vec4(finalColor, 1.0);
}
