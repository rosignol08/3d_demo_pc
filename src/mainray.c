#include "raylib.h"
#include "raymath.h"
#include <math.h>

// --- MATHS (Identique) ---
typedef struct {
    bool hit;
    float dist;
    Vector3 point;
    Vector3 normal;
} HitInfo;

HitInfo IntersectSphere(Vector3 ro, Vector3 rd, Vector3 sPos, float sRad) {
    HitInfo info = { 0 }; info.dist = 10000.0f;
    Vector3 oc = Vector3Subtract(ro, sPos);
    float b = Vector3DotProduct(oc, rd);
    float c = Vector3DotProduct(oc, oc) - sRad*sRad;
    float h = b*b - c;
    if (h < 0.0f) return info;
    float t = -b - sqrtf(h);
    if (t > 0.0f) {
        info.hit = true; info.dist = t;
        info.point = Vector3Add(ro, Vector3Scale(rd, t));
        info.normal = Vector3Normalize(Vector3Subtract(info.point, sPos));
    }
    return info;
}

HitInfo IntersectPlane(Vector3 ro, Vector3 rd, float planeY) {
    HitInfo info = { 0 }; info.dist = 10000.0f;
    if (fabs(rd.y) > 0.001f) {
        float t = (planeY - ro.y) / rd.y;
        if (t > 0.0f) {
            info.hit = true; info.dist = t;
            info.point = Vector3Add(ro, Vector3Scale(rd, t));
            info.normal = (Vector3){ 0, 1, 0 };
        }
    }
    return info;
}

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib - Mouse Ray Tracing");

    Shader shader = LoadShader(0, "dark.fs"); // Nom du nouveau shader
    
    int resLoc = GetShaderLocation(shader, "resolution");
    int camPosLoc = GetShaderLocation(shader, "camPos");
    int camTargetLoc = GetShaderLocation(shader, "camTarget");
    int lightPosLoc = GetShaderLocation(shader, "lightPos");
    int lightActiveLoc = GetShaderLocation(shader, "lightActive");

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    SetShaderValue(shader, resLoc, resolution, SHADER_UNIFORM_VEC2);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Définition de la scène physique (doit matcher le shader)
    Vector3 spherePos = { 0.0f, 0.0f, 0.0f };
    float sphereRadius = 1.0f;

    SetTargetFPS(6000);

    while (!WindowShouldClose())
    {
        // Contrôle caméra (Clic droit seulement)
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) UpdateCamera(&camera, CAMERA_ORBITAL);

        // 1. LE RAYON DE LA SOURIS (Ray Casting)
        // Ce rayon part de la caméra et passe par le pixel de la souris
        Ray mouseRay = GetMouseRay(GetMousePosition(), camera);

        // 2. CALCUL DES COLLISIONS CPU
        HitInfo hitS = IntersectSphere(mouseRay.position, mouseRay.direction, spherePos, sphereRadius);
        HitInfo hitP = IntersectPlane(mouseRay.position, mouseRay.direction, -1.0f);
        
        HitInfo closestHit = {0};
        closestHit.dist = 100.0f;
        bool hasHit = false;

        if (hitS.hit && hitS.dist < closestHit.dist) { closestHit = hitS; hasHit = true; }
        if (hitP.hit && hitP.dist < closestHit.dist) { closestHit = hitP; hasHit = true; }

        // 3. ENVOI AU SHADER
        float camP[3] = { camera.position.x, camera.position.y, camera.position.z };
        float camT[3] = { camera.target.x, camera.target.y, camera.target.z };
        float lPos[3] = { closestHit.point.x, closestHit.point.y, closestHit.point.z };
        float lActive = hasHit ? 1.0f : 0.0f;

        SetShaderValue(shader, camPosLoc, camP, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, camTargetLoc, camT, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, lightPosLoc, lPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader, lightActiveLoc, &lActive, SHADER_UNIFORM_FLOAT);

        BeginDrawing();
            ClearBackground(BLACK);

            // A. DESSIN DU SHADER (La scène obscure)
            BeginShaderMode(shader);
                DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            EndShaderMode();

            // B. VISUALISATION DES RAYONS (Overlay)
            BeginMode3D(camera);
                BeginBlendMode(BLEND_ADDITIVE);

                // --- 1. LE RAYON PRIMAIRE (De la caméra à l'objet) ---
                // On ne le dessine que s'il touche, sinon on dessine un trait vers l'infini
                Vector3 endPoint = hasHit ? closestHit.point : Vector3Add(mouseRay.position, Vector3Scale(mouseRay.direction, 50.0f));
                
                // Dessin d'un "faisceau" (plusieurs lignes pour l'épaisseur)
                Color beamCol = SKYBLUE;
                beamCol.a = 150;
                DrawLine3D(Vector3Add(camera.position, (Vector3){0,-0.1,0}), endPoint, beamCol); // Léger décalage pour voir le rayon sortir
                
                // --- 2. LE REBOND (Si collision) ---
                if (hasHit) {
                    // Point d'impact
                    DrawSphere(closestHit.point, 0.05f, WHITE);
                    
                    // Calcul du vecteur réfléchi : R = I - 2(N.I)N
                    Vector3 incident = mouseRay.direction;
                    Vector3 normal = closestHit.normal;
                    Vector3 reflected = Vector3Reflect(incident, normal);
                    
                    // Dessin du rayon réfléchi (Rouge pour bien le voir)
                    Vector3 bounceEnd = Vector3Add(closestHit.point, Vector3Scale(reflected, 5.0f)); // 5 mètres de long
                    
                    // On vérifie si le rebond tape autre chose (ex: rebond du sol vers la sphère)
                    // C'est ça le vrai raytracing !
                    HitInfo bounceS = IntersectSphere(closestHit.point, reflected, spherePos, sphereRadius);
                    if(bounceS.hit && bounceS.dist > 0.01f) {
                        bounceEnd = bounceS.point; // Le rebond s'arrête sur l'obstacle suivant
                        DrawSphere(bounceEnd, 0.03f, RED); // Impact secondaire
                    }
                    
                    DrawLine3D(closestHit.point, bounceEnd, RED);
                }

                EndBlendMode();
            EndMode3D();

            DrawFPS(10, 10);
            DrawText("Souris = Rayon | Clic Droit = Caméra", 10, 30, 20, GRAY);
            if(!hasHit) DrawText("MISS", 10, 50, 20, DARKGRAY);

        EndDrawing();
    }

    UnloadShader(shader);
    CloseWindow();

    return 0;
}