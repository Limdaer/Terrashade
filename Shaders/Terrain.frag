#version 460 core

out vec4 FragColor;

in vec3 FragPos;  
in vec3 Normal;  

// Uniformy pro osvětlení
uniform vec3 lightDir = normalize(vec3(-1.0, -1.0, -1.0)); // Směr světla (např. slunce)
uniform vec3 lightColor = vec3(1.0, 1.0, 0.9);  // Sluneční světlo (teplejší tón)
uniform vec3 terrainColor = vec3(0.2, 0.6, 0.2); // Barva terénu

// Parametry Phongova osvětlení
uniform float ambientStrength = 0.2;  // Ambientní světlo
uniform float diffuseStrength = 0.8;  // Difuzní světlo
uniform float specularStrength = 0.5; // Spekulární odraz
uniform float shininess = 32.0;       // Lesklost materiálu

// Kamera (pro spekulární světlo)
uniform vec3 viewPos;

void main() {
    // Normalizace normály
    vec3 norm = normalize(Normal);

    // AMBIENTNÍ SVĚTLO (měkké základní světlo)
    vec3 ambient = ambientStrength * lightColor;

    // DIFUZNÍ SVĚTLO (osvětlení podle úhlu normály vůči světlu)
    float diff = max(dot(norm, -lightDir), 0.0);
    vec3 diffuse = diffuseStrength * diff * lightColor;

    // SPEKULÁRNÍ SVĚTLO (lesklé odrazy)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    // Finální výpočet barvy
    vec3 result = (ambient + diffuse + specular) * terrainColor;
    FragColor = vec4(result, 1.0);
}
