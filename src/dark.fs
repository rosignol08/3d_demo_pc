#version 330

out vec4 finalColor;

uniform vec2 resolution;
uniform vec3 camPos;
uniform vec3 camTarget;

// LUMIÈRE (Position de l'impact du laser)
uniform vec3 lightPos;      
uniform float lightActive;  

const int MAX_STEPS = 1000;
const float MAX_DIST = 1000.0;
const float SURF_DIST = 0.001;

// --- GEOMETRIE SDF ---
float sdSphere(vec3 p, float s) { return length(p) - s; }
float sdPlane(vec3 p) { return p.y + 1.0; }

float GetDist(vec3 p) {
    // Union de la sphère et du sol
    return min(sdSphere(p, 1.0), sdPlane(p));
}

// --- CALCUL DES OMBRES (Le retour !) ---
// ro: Ray Origin (point sur la surface), rd: Ray Direction (vers la lumière)
// maxDist: Distance jusqu'à la lumière (pour ne pas vérifier derrière la lampe)
float GetShadow(vec3 ro, vec3 rd, float k, float maxDist) {
    float res = 1.0;
    float t = 0.02; // On commence un peu décalé pour éviter l'auto-intersection
    
    for(int i = 0; i < 50; i++) {
        float h = GetDist(ro + rd * t);
        
        if(h < 0.001) return 0.0; // On a touché un obstacle -> Ombre totale
        
        // Soft Shadow (Pénombre)
        res = min(res, k * h / t);
        
        t += h;
        // Si on a dépassé la lumière, on arrête (pas d'ombre causée par des objets derrière la lumière)
        if(t > maxDist) break; 
    }
    return clamp(res, 0.0, 1.0);
}

vec3 GetNormal(vec3 p) {
    float d = GetDist(p);
    vec2 e = vec2(0.001, 0);
    vec3 n = d - vec3(GetDist(p-e.xyy), GetDist(p-e.yxy), GetDist(p-e.yyx));
    return normalize(n);
}

float RayMarch(vec3 ro, vec3 rd) {
    float dO = 0.0;
    for(int i=0; i<MAX_STEPS; i++) {
        vec3 p = ro + rd*dO;
        float dS = GetDist(p);
        dO += dS;
        if(dO>MAX_DIST || dS<SURF_DIST) break;
    }
    return dO;
}

void main() {
    vec2 uv = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
    // uv.y = -uv.y; // Décommenter si nécessaire

    vec3 ro = camPos;
    vec3 f = normalize(camTarget - ro);
    vec3 r = normalize(cross(vec3(0,1,0), f));
    vec3 u = cross(f,r);
    vec3 rd = normalize(f + r*uv.x + u*uv.y);

    float d = RayMarch(ro, rd);
    vec3 col = vec3(0.0); // Noir par défaut

    if(d < MAX_DIST && lightActive > 0.2) {
        vec3 p = ro + rd * d;
        vec3 n = GetNormal(p);

        // Vecteur vers la lumière (l'impact du laser)
        vec3 lightDir = lightPos - p;
        float distToLight = length(lightDir);
        lightDir = normalize(lightDir); // On normalise après avoir eu la longueur

        // 1. CALCUL DE L'OMBRE
        // On lance un rayon depuis le point visible (p) vers la lumière (lightPos)
        // 16.0 = dureté de l'ombre (plus grand = plus net)
        float shadow = GetShadow(p + n * 0.02, lightDir, 16.0, distToLight);

        // Si on est à l'ombre, on n'éclaire pas
        if(shadow > 0.01) {
            // 2. ATTENUATION (Lumière ponctuelle)
            // La lumière est très forte au point d'impact et faiblit vite
            float atten = 1.0 / (1.0 + distToLight * distToLight * 5.0);
            
            // 3. DIFFUSE (Loi de Lambert)
            float diff = max(dot(n, lightDir), 0.0);
            
            // 4. COULEUR FINALE
            vec3 lightColor = vec3(0.2, 0.6, 1.0); // Bleu laser
            
            // Formule : Couleur * Intensité * Ombre * (Diffuse + AmbientMinimale)
            col = lightColor * atten * shadow * (diff + 0.05);
            
            // Petit halo supplémentaire pour simuler la source lumineuse elle-même
            // Si le pixel est très proche de la source, il brille fort
            //if(distToLight < 0.1) col += vec3(1.0); 
        }
    }

    // Gamma correction
    col = pow(col, vec3(1.0/2.2));
    finalColor = vec4(col, 1.0);
}