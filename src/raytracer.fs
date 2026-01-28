#version 330

out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 camPos;
uniform vec3 camTarget;
uniform float time; // Nécessaire pour l'aléatoire (grain)

// --- PARAMÈTRES QUALITÉ ---
#define MAX_BOUNCES 1000     // Nombre de rebonds de la lumière
#define SAMPLES_PER_PIXEL 10 // Plus c'est haut, moins il y a de bruit (mais plus lent)

// --- CONSTANTES ---
const float PI = 3.14159265359;
const float FAR = 1000.0;

// --- GÉNÉRATEUR ALÉATOIRE (RNG) ---
// Essentiel pour le Path Tracing (Monte Carlo)
float seed;
float rand() {
    return fract(sin(seed++) * 43758.5453123);
}

// --- INTERSECTIONS ANALYTIQUES (Ultra Rapides) ---

// Intersection Sphère (Retourne la distance ou FAR si raté)
float iSphere(vec3 ro, vec3 rd, vec4 sph) {
    vec3 oc = ro - sph.xyz;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - sph.w * sph.w;
    float h = b * b - c;
    if (h < 0.0) return FAR;
    float t = -b - sqrt(h);
    return t > 0.0 ? t : FAR;
}

// Intersection Cube (Box) - Méthode "Slab"
float iBox(vec3 ro, vec3 rd, vec3 boxSize, out vec3 outNormal) {
    vec3 m = 1.0 / rd;
    vec3 n = m * ro;
    vec3 k = abs(m) * boxSize;
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max(max(t1.x, t1.y), t1.z);
    float tF = min(min(t2.x, t2.y), t2.z);
    if (tN > tF || tF < 0.0) return FAR;
    
    // Calcul de la normale du cube au point d'impact
    vec3 p = ro + rd * tN;
    vec3 s = step(abs(p), boxSize); 
    // Petite astuce pour chopper la normale sans trop de calculs
    outNormal = -sign(rd) * step(vec3(tN), t1);
    
    return tN;
}

// Intersection Plan (Sol)
float iPlane(vec3 ro, vec3 rd) {
    // Plan à y = -1.0
    float t = -(ro.y + 1.0) / rd.y;
    return t > 0.0 ? t : FAR;
}

// --- LOGIQUE DE LA SCÈNE ---
// Retourne: t (distance), n (normale), matID (id matière)
vec3 map(vec3 ro, vec3 rd, out vec3 n, out int matID) {
    float t = FAR;
    matID = 0; // 0 = Ciel
    
    // 1. Test Sphère
    // Sphère au centre, rayon 1.0
    float tSph = iSphere(ro, rd, vec4(0.0, 0.0, 0.0, 1.0));
    if (tSph < t) { 
        t = tSph; 
        n = (ro + rd * t) / 1.0; // Normale sphère unit = position locale
        matID = 1; // 1 = Objet
    }
    
    // 2. Test Cube (Décommenter pour activer, commenter la sphère au dessus pour tester)
    // vec3 boxN;
    // float tBox = iBox(ro, rd, vec3(0.75), boxN);
    // if (tBox < t) { t = tBox; n = boxN; matID = 1; }

    // 3. Test Sol
    float tPlane = iPlane(ro, rd);
    if (tPlane < t) {
        t = tPlane;
        n = vec3(0.0, 1.0, 0.0);
        matID = 2; // 2 = Sol
    }
    
    return vec3(t);
}

// --- ECLAIRAGE (CIEL) ---
vec3 getSky(vec3 rd) {
    // Un dégradé bleu-gris vers blanc (HDRI simulé)
    float t = 0.5 * (rd.y + 1.0);
    return mix(vec3(1.0), vec3(0.5, 0.7, 1.0), t) * 1.5; // *1.5 pour l'intensité
}

// --- PATH TRACING KERNEL ---
vec3 getRadiance(vec3 ro, vec3 rd) {
    vec3 col = vec3(0.0);
    vec3 throughput = vec3(1.0); // Quantité de lumière que le rayon transporte
    
    for (int i = 0; i < MAX_BOUNCES; i++) {
        vec3 n;
        int matID;
        vec3 hitRes = map(ro, rd, n, matID);
        float t = hitRes.x;
        
        if (t >= FAR) {
            // Le rayon part dans le ciel
            col += throughput * getSky(rd);
            break;
        }
        
        // --- ON A TOUCHÉ QUELQUE CHOSE ---
        vec3 p = ro + rd * t;
        
        // Couleur de l'objet (Albedo)
        vec3 albedo = vec3(0.0);
        
        if (matID == 1) { 
            // SPHÈRE/CUBE : Rouge vif (pour voir le rebond de couleur)
            albedo = vec3(0.9, 0.1, 0.1); 
        } 
        else if (matID == 2) { 
            // SOL : Gris clair avec damier subtil
            float check = mod(floor(p.x) + floor(p.z), 2.0);
            albedo = vec3(0.5) + check * 0.2;
        }
        
        // Mise à jour de la lumière transportée
        throughput *= albedo;
        
        // --- REBOND (Diffuse Reflection) ---
        // On choisit une direction aléatoire dans l'hémisphère orienté selon la normale
        // Méthode Cosine Weighted Sampling pour moins de bruit
        float r1 = rand();
        float r2 = rand();
        float phi = 2.0 * PI * r1;
        float sq = sqrt(r2);
        
        vec3 u = normalize(cross(abs(n.x) > 0.1 ? vec3(0, 1, 0) : vec3(1, 0, 0), n));
        vec3 v = cross(n, u);
        vec3 randomDir = normalize(u * cos(phi) * sq + v * sin(phi) * sq + n * sqrt(1.0 - r2));
        
        // Préparer le prochain rayon
        ro = p + n * 0.001; // Offset pour éviter l'auto-intersection
        rd = randomDir;
        
        // Roulette Russe (Optimisation) : Tue le rayon s'il est trop sombre
        if (i > 1) {
            float p = max(throughput.r, max(throughput.g, throughput.b));
            if (rand() > p) break;
            throughput /= p;
        }
    }
    return col;
}

void main() {
    // Initialisation du Seed aléatoire par pixel + temps
    vec2 q = gl_FragCoord.xy / resolution.xy;
    seed = (q.x + q.y * 1000.0) * time; // Changer le seed à chaque frame

    // UVs
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    //uv.y = -uv.y; // Décommenter si inversé
    
    // Caméra
    vec3 ro = camPos;
    vec3 f = normalize(camTarget - ro);
    vec3 r = normalize(cross(vec3(0, 1, 0), f));
    vec3 u = cross(f, r);
    
    // Accumulation (Multi-Sampling)
    vec3 finalCol = vec3(0.0);
    
    for(int i = 0; i < SAMPLES_PER_PIXEL; i++) {
        // Anti-Aliasing (Jittering) : On décale très légèrement le rayon dans le pixel
        vec2 off = vec2(rand() - 0.5, rand() - 0.5) / resolution.y;
        vec3 rd = normalize(f + r * (uv.x + off.x) + u * (uv.y + off.y));
        
        finalCol += getRadiance(ro, rd);
    }
    
    finalCol /= float(SAMPLES_PER_PIXEL);
    
    // Gamma Correction
    finalCol = pow(finalCol, vec3(1.0/2.2));
    
    finalColor = vec4(finalCol, 1.0);
}