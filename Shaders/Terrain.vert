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
uniform float heightScale = 10.0;
uniform float edgeSharpness = 3.5;

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

// Simplex Noise 2D
vec2 grad2(int hash) {
    const vec2 G[8] = vec2[8](
        vec2(1,1), vec2(-1,1), vec2(1,-1), vec2(-1,-1),
        vec2(1,0), vec2(-1,0), vec2(0,1), vec2(0,-1)
    );
    return G[hash & 7];
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

float simplexNoise(vec2 v) {
    const float F2 = 0.36602540378; // (sqrt(3)-1)/2
    const float G2 = 0.2113248654;  // (3-sqrt(3))/6

    vec2 i = floor(v + dot(v, vec2(F2, F2)));
    vec2 x0 = v - (i - dot(i, vec2(G2, G2)));

    vec2 i1 = (x0.x > x0.y) ? vec2(1, 0) : vec2(0, 1);
    vec2 x1 = x0 - i1 + vec2(G2, G2);
    vec2 x2 = x0 - vec2(1.0, 1.0) + vec2(2.0 * G2, 2.0 * G2);

    int h0 = hash(int(i.x), int(i.y));
    int h1 = hash(int(i.x + i1.x), int(i.y + i1.y));
    int h2 = hash(int(i.x + 1), int(i.y + 1));

    vec2 g0 = grad2(h0);
    vec2 g1 = grad2(h1);
    vec2 g2 = grad2(h2);

    float t0 = max(0.5 - dot(x0, x0), 0.0);
    float t1 = max(0.5 - dot(x1, x1), 0.0);
    float t2 = max(0.5 - dot(x2, x2), 0.0);

    t0 *= t0; t0 *= t0;
    t1 *= t1; t1 *= t1;
    t2 *= t2; t2 *= t2;

    return 70.0 * (t0 * dot(g0, x0) + t1 * dot(g1, x1) + t2 * dot(g2, x2));
}

// Voronoi Noise generování výšky
float voronoiNoise(vec2 pos, float edgeSharpness) {
    vec2 cell = floor(pos); // Najdeme buňku, ve které jsme
    vec2 localPos = fract(pos); // Relativní pozice uvnitř buňky

    float minDist = 1.0; // Nejmenší vzdálenost (počáteční hodnota)

    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
            vec2 neighbor = vec2(x, y); // Sousední buňky
            vec2 point = vec2(hash(int(cell.x) + x, int(cell.y) + y) & 255) / 255.0; // Náhodné centrum buňky
            float dist = length(neighbor + point - localPos); // Vzdálenost od tohoto bodu
            minDist = min(minDist, dist); // Uložíme nejbližší vzdálenost
        }
    }
    //smoothstep nastavuje hladší přechod mezi hodnotami (edge1,edge2,x)
    float smoothHeight = smoothstep(0.3,1.0,minDist);
    return -pow(smoothHeight,edgeSharpness);
}

// Hlavní funkce vertex shaderu
void main() {
    // Výpočet výšky terénu pomocí Perlin Noise / FBM
    //float height = fbm(aPos.xz * 0.1) * heightScale;
    // Výpočet výšky terénu pomocí Voronoi Diagram + Perlin Noise
    float height = (fbm(aPos.xz * 0.1) + voronoiNoise(aPos.xz * 0.1, edgeSharpness)) * heightScale;
    //Výpočet výšky terénu pomocí Simplex Noise
    //float height = -pow((simplexNoise(aPos.xz * 0.1) * 0.5 + 0.5), 2.5) * heightScale;
    //Výpočet výšky terénu pomocí Simplex Noise a Voronoi
    //float height = (-pow((simplexNoise(aPos.xz * 0.1) * 0.5 + 0.5), 2.5) + voronoiNoise(aPos.xz * 0.1, edgeSharpness)) * heightScale;

    vec3 newPosition = vec3(aPos.x, height, aPos.z);

    // Výpočet normály Perlin
    //rozdíl výšek mezi bodem a bodem o +1 dál
    //float dx = fbm((aPos.xz + vec2(1.0, 0.0)) * 0.1) * heightScale - height;
    //float dz = fbm((aPos.xz + vec2(0.0, 1.0)) * 0.1) * heightScale - height;

    // Výpočet normály Perlin + Voronoi - rozdíl výšek mezi bodem a bodem o +1 dál
    float dx = ((fbm((aPos.xz + vec2(1.0, 0.0)) * 0.1) 
    + voronoiNoise((aPos.xz + vec2(1.0, 0.0)) * 0.1, edgeSharpness)) * heightScale) - height;
    float dz = ((fbm((aPos.xz + vec2(0.0, 1.0)) * 0.1) 
    + voronoiNoise((aPos.xz + vec2(0.0, 1.0)) * 0.1, edgeSharpness)) * heightScale) - height;

    // Výpočet normály Simplex Noise
    //float dx = -pow((simplexNoise((aPos.xz + vec2(1.0, 0.0)) * 0.1) * 0.5 + 0.5), 2.5) * heightScale - height;
    //float dz = -pow((simplexNoise((aPos.xz + vec2(0.0, 1.0)) * 0.1) * 0.5 + 0.5), 2.5) * heightScale - height;

    // Výpočet normály Simplex Noise a Voronoi
    //float dx = ((-pow((simplexNoise((aPos.xz + vec2(1.0, 0.0)) * 0.1) * 0.5 + 0.5), 2.5) 
    //+ voronoiNoise((aPos.xz + vec2(1.0, 0.0)) * 0.1, edgeSharpness)) * heightScale) - height;
    //float dz = ((-pow((simplexNoise((aPos.xz + vec2(0.0, 1.0)) * 0.1) * 0.5 + 0.5), 2.5) 
    //+ voronoiNoise((aPos.xz + vec2(0.0, 1.0)) * 0.1, edgeSharpness)) * heightScale) - height;


    vec3 calculatedNormal = normalize(vec3(-dx, 1.0, -dz));

    // Předání do fragment shaderu
    FragPos = vec3(model * vec4(newPosition, 1.0));
    Normal = mat3(transpose(inverse(model))) * calculatedNormal;

    gl_Position = projection * view * model * vec4(newPosition, 1.0);
}