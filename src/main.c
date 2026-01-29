#include "raylib.h"
#include "raymath.h"
#include <stdio.h> // Pour NULL

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION 330
#else
    #define GLSL_VERSION 100
#endif
#define NB_SCENES 5 //TODO changer si c'est pas le cas

// --- 1. DÉFINITION DE LA STRUCTURE DE SCÈNE ---
typedef struct {
    void (*Init)(void);
    void (*Update)(float time, float delta);
    void (*Draw)(void);
    void (*Unload)(void);
    float duration; // Durée de la scène en secondes
} Scene;

// --- VARIABLES GLOBALES (Accessibles par toutes les scènes) ---
Camera camera = { 0 };
float globalTime = 0.0f;
Shader shaders[NB_SCENES] = {0};
// -------------------------------------------------------------------------
// --- SCÈNE 1 : LE CUBE ET LE DAMIER (Ton code d'origine) ---
// -------------------------------------------------------------------------
Model modelCube;
Model modelsol;
Texture2D textureDamier;
int timeParamLoc;
const char * gouraud_vs =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec3 vertexNormal;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec4 vertexColor;\n"
    
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform vec3 lightPos;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec4 colDiffuse;\n"
    
    "out vec4 fragColor;\n"
    
    "void main() {\n"
    "    vec3 worldPos = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    vec3 N = normalize(vec3(matModel * vec4(vertexNormal, 0.0)));\n"
    "    vec3 L = normalize(lightPos - worldPos);\n"
    
    "    // Ambiante\n"
    "    float ambientStrength = 0.1;\n"
    "    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);\n"
    
    "    // Diffuse\n"
    "    float diff = max(dot(N, L), 0.0);\n"
    "    vec3 diffuse = diff * vec3(1.0, 1.0, 1.0);\n"
    
    "    // Speculaire (Gouraud style)\n"
    "    float specularStrength = 0.5;\n"
    "    vec3 V = normalize(viewPos - worldPos);\n"
    "    vec3 R = reflect(-L, N);\n"
    "    float spec = pow(max(dot(V, R), 0.0), 16.0);\n"
    "    vec3 specular = specularStrength * spec * vec3(1.0, 1.0, 1.0);\n"
    
    "    vec3 result = (ambient + diffuse) * colDiffuse.rgb + specular;\n"
    "    fragColor = vec4(result, 1.0);\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

const char* gouraud_fs = 
    "#version 330\n"
    "in vec4 fragColor;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    finalColor = fragColor;\n"
    "}\n";
Shader shader_eclairage; //en global pour qu'il soit effacé apres
int lightPosLoc;
int viewPosLoc;
void InitScene1(void) {
    // Caméra spécifique à la scène 1
    camera.position = (Vector3){ 4.0f, 4.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 0.5f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;

    // Génération texture
    int width = 256; int height = 256; int squareSize = 32;
    Color colors[2] = { BLACK, WHITE };
    Image damier = GenImageColor(width, height, BLANK);
    
    // Ton algo de damier optimisé pour ImageDrawPixel (plus sûr que l'accès pointeur direct parfois)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int colorIndex = ((x / squareSize) + (y / squareSize)) % 2;
            ImageDrawPixel(&damier, x, y, colors[colorIndex]);
        }
    }
    shader_eclairage = LoadShaderFromMemory(gouraud_vs, gouraud_fs);
    lightPosLoc = GetShaderLocation(shader_eclairage, "lightPos");
    viewPosLoc = GetShaderLocation(shader_eclairage, "viewPos");
    textureDamier = LoadTextureFromImage(damier);
    UnloadImage(damier);
    Mesh sol = GenMeshPlane(10.0f, 10.0f, 100.0f, 100.0f);
    modelsol = LoadModelFromMesh(sol);
    Mesh mesh = GenMeshCube(2.0f, 2.0f, 2.0f);
    modelCube = LoadModelFromMesh(mesh);
    modelCube.materials[0].maps[MATERIAL_MAP_ALBEDO].color = RED;
    modelCube.materials[0].shader = shader_eclairage;
    modelsol.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = textureDamier;
    modelsol.materials[0].shader = shader_eclairage;
}

void UpdateScene1(float time, float delta) {
    // Rotation automatique caméra (effet demoscene)
    float rotSpeed = 0.5f;
    camera.position.x = sin(time * rotSpeed) * 5.0f;
    camera.position.z = cos(time * rotSpeed) * 5.0f;
    UpdateCamera(&camera, CAMERA_PERSPECTIVE);
}

void DrawScene1(void) {
    DrawGrid(10, 1.0f);
    DrawModel(modelCube, (Vector3){0, 1.0f, 0}, 1.0f, WHITE);
    DrawModel(modelsol, (Vector3){0, 0.0f, 0}, 1.0f, WHITE);
}

void UnloadScene1(void) {
    UnloadTexture(textureDamier);
    UnloadModel(modelCube);
}

// -------------------------------------------------------------------------
// --- SCÈNE 2 : UN TUNNEL TRON (Exemple de changement) ---
// -------------------------------------------------------------------------
// On utilise des variables statiques pour éviter les conflits de noms globaux
static const int MAX_COLUMNS = 20;

void InitScene2(void) {
    camera.position = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.target = (Vector3){ 0.0f, 1.0f, 10.0f }; // Regarde au loin
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
}

void UpdateScene2(float time, float delta) {
    // Avance la caméra tout droit
    camera.position.z = time * 5.0f; 
    camera.target.z = camera.position.z + 10.0f;
    UpdateCamera(&camera, CAMERA_PERSPECTIVE);
}

void DrawScene2(void) {
    // Dessine un tunnel infini en répétant des cubes
    float spacing = 2.0f;
    int visibleDistance = 20;
    
    // On calcule où commencer à dessiner par rapport à la caméra
    int startZ = (int)(camera.position.z / spacing);

    for (int i = 0; i < visibleDistance; i++) {
        float z = (startZ + i) * spacing;
        
        // Effet de couleur pulsée
        Color col = (i % 2 == 0) ? RED : DARKBLUE;
        
        // Dessine des arches
        DrawCube((Vector3){-2.0f, 1.0f, z}, 0.5f, 2.0f, 0.5f, col); // Gauche
        DrawCube((Vector3){ 2.0f, 1.0f, z}, 0.5f, 2.0f, 0.5f, col); // Droite
        DrawCube((Vector3){ 0.0f, 2.5f, z}, 4.5f, 0.5f, 0.5f, col); // Haut
    }
}

void UnloadScene2(void) {
    // Rien à décharger ici (pas de textures chargées)
}

// -------------------------------------------------------------------------
// --- MAIN ---
// -------------------------------------------------------------------------
int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 450;
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Demoscene Engine - Raylib");

    // --- CONFIGURATION DU SHADER POST-PROCESS (VHS) ---
    Shader shader1 = LoadShader(0, "vhs.fs");

    shaders[0] = shader1;
    
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);
    
    // --- TIMELINE / PLAYLIST DES SCÈNES ---
    Scene scenes[] = {
        { InitScene1, UpdateScene1, DrawScene1, UnloadScene1, 5.0f }, // Scène 1 dure 5s
        { InitScene2, UpdateScene2, DrawScene2, UnloadScene2, 8.0f }  // Scène 2 dure 8s
    };
    int sceneCount = sizeof(scenes) / sizeof(scenes[0]);
    int currentSceneIndex = 0;
    float sceneTimer = 0.0f; // Temps écoulé dans la scène actuelle

    // Init de la première scène
    scenes[currentSceneIndex].Init();

    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        float delta = GetFrameTime();
        globalTime = GetTime();
        sceneTimer += delta;

        // --- GESTION DU CHANGEMENT DE SCÈNE ---
        if (sceneTimer > scenes[currentSceneIndex].duration || IsKeyPressed(KEY_SPACE)) {
            // 1. Nettoyer l'ancienne scène
            scenes[currentSceneIndex].Unload();
            
            // 2. Passer à la suivante (boucle au début si fini)
            currentSceneIndex = (currentSceneIndex + 1) % sceneCount;
            sceneTimer = 0.0f;
            
            // 3. Charger la nouvelle
            scenes[currentSceneIndex].Init();
        }

        // --- UPDATE ---
        // On appelle l'Update de la scène active
        scenes[currentSceneIndex].Update(globalTime, delta);

        //update du shader
        if (currentSceneIndex == 1){
            timeParamLoc = GetShaderLocation(shaders[1], "time");
            lightPosLoc = GetShaderLocation(shader_eclairage, "lightPos");
            viewPosLoc = GetShaderLocation(shader_eclairage, "viewPos");
            SetShaderValue(shaders[0], timeParamLoc, &globalTime, SHADER_UNIFORM_FLOAT);}

        // --- DRAW ---
        BeginTextureMode(target);
            ClearBackground(BLACK); // Fond noir pour le cinéma
            BeginMode3D(camera);
                // On appelle le Draw de la scène active
                scenes[currentSceneIndex].Draw();
            EndMode3D();
        EndTextureMode();
        
        BeginDrawing();
            ClearBackground(BLACK);
            
            BeginShaderMode(shaders[currentSceneIndex]);
                // Dessin de la Render Texture (Y inversé)
                DrawTextureRec(target.texture, 
                              (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, 
                              (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            
            // UI Debug
            DrawFPS(10, 10);
            DrawText(TextFormat("SCENE: %d / TIME: %.2f", currentSceneIndex + 1, sceneTimer), 10, 30, 20, GREEN);
            DrawText("SPACE to skip scene", 10, 50, 10, GRAY);
        EndDrawing();
    }

    // Cleanup Final
    scenes[currentSceneIndex].Unload(); // Décharger la dernière scène active
    UnloadRenderTexture(target);
    //UnloadShader(shader);
    // Décharger tous les shaders
    for (int i = 0; i < sizeof(shaders) / sizeof(shaders[0]); i++) {
        UnloadShader(shaders[i]);
    }
    UnloadShader(shader_eclairage);
    CloseWindow();

    return 0;
}