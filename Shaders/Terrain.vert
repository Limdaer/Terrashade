#version 460 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Výstup do fragment shaderu
out vec3 FragPos;  
out vec3 Normal;

// Uniformy pro řízení terénu
uniform int octaves = 6;  // Počet hladin šumu
uniform float persistence = 0.3; // Jak moc se snižuje amplituda s další hladinou
uniform float lacunarity = 2.0;  // Jak moc se zvyšuje frekvence s další hladinou
uniform float heightScale = 50.0;

//Pomocí kubické interpolace - 
// Funkce pro fade efekt - verze pro float
float fade(float t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Přetížení funkce fade pro vec2
vec2 fade(vec2 t) {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

// Lineární interpolace mezi dvěma hodnotami - "míchá 2 hodnoty podle váhy"
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// Generování gradientu na základě hash hodnoty 
//Výběr náhodného směru gradientu pomocí h, generuje lokální variace, které se pak interpolují
float grad(int hash, float x, float y) {
    int h = hash & 7; //pouze spodní 3 bity
    float u = h < 4 ? x : y;
    float v = h < 2 ? y : x;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// Hashovací funkce pro pseudonáhodné hodnoty - není potřeba tabulka permutací
int hash(int x, int y) {
    x = (x << 13) ^ x;
    y = (y << 17) ^ y;
    return ((x * y * 15731 + 789221) ^ (x + y * 1376312589)) & 255;
}

// Výpočet Perlinova šumu
//Najdeme čtvercovou buňku do které bod patří, získáme relativní pozice uvnitř buňky
//fade() na hladší přechody
//4 hash hodnoty a spočítáme gradientové vlivy na rohy
//interpolujeme hodnoty mezi rohy pomocí lerp()
//vrátíme výslednou výšku
float perlinNoise(vec2 pos) {
    ivec2 cell = ivec2(floor(pos));
    vec2 localPos = fract(pos);

    vec2 fadeXY = fade(localPos);

    int h00 = hash(cell.x, cell.y);
    int h10 = hash(cell.x + 1, cell.y);
    int h01 = hash(cell.x, cell.y + 1);
    int h11 = hash(cell.x + 1, cell.y + 1);

    float n00 = grad(h00, localPos.x, localPos.y);
    float n10 = grad(h10, localPos.x - 1.0, localPos.y);
    float n01 = grad(h01, localPos.x, localPos.y - 1.0);
    float n11 = grad(h11, localPos.x - 1.0, localPos.y - 1.0);

    float nx0 = lerp(n00, n10, fadeXY.x);
    float nx1 = lerp(n01, n11, fadeXY.x);
    return lerp(nx0, nx1, fadeXY.y);
}

// Fractal Brownian Motion (kombinace více hladin Perlin Noise)
float fbm(vec2 pos) {
    float total = 0.0;
    float amplitude = 1.0;
    float frequency = 0.5;
    float maxValue = 0.0;

    for (int i = 0; i < octaves; i++) {
        total += perlinNoise(pos * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= persistence;  // Snižujeme sílu šumu
        frequency *= lacunarity;   // Zvyšujeme frekvenci
    }

    return total / maxValue; // Normalizace
}

// Hlavní funkce vertex shaderu
void main() {
    // Výpočet výšky terénu
    float height = fbm(aPos.xz * 0.1) * heightScale;
    vec3 newPosition = vec3(aPos.x, height, aPos.z);

    // Výpočet normály
    //rozdíl výšek mezi bodem a bodem o +1 dál
    float dx = fbm((aPos.xz + vec2(1.0, 0.0)) * 0.1) * heightScale - height;
    float dz = fbm((aPos.xz + vec2(0.0, 1.0)) * 0.1) * heightScale - height;
    vec3 calculatedNormal = normalize(vec3(-dx, 1.0, -dz));

    // Předání do fragment shaderu
    FragPos = vec3(model * vec4(newPosition, 1.0));
    Normal = mat3(transpose(inverse(model))) * calculatedNormal;

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
}