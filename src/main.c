#include "raylib.h"
#include <math.h>

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "Raylib - Realtime Path Tracing");

    Shader shader = LoadShader(0, "shad.fs");

    int resLoc = GetShaderLocation(shader, "resolution");
    int camPosLoc = GetShaderLocation(shader, "camPos");
    int camTargetLoc = GetShaderLocation(shader, "camTarget");
    int timeLoc = GetShaderLocation(shader, "time"); // Nouveau !

    float resolution[2] = { (float)screenWidth, (float)screenHeight };
    
    Vector3 camPos = { 2.5f, 2.0f, 3.0f }; // Caméra un peu en hauteur
    Vector3 camTarget = { 0.0f, 0.0f, 0.0f };

    SetShaderValue(shader, resLoc, resolution, SHADER_UNIFORM_VEC2);

    SetTargetFPS(6000);

    while (!WindowShouldClose())
    {
        // Mise à jour du temps pour le bruit aléatoire
        float time = (float)GetTime();
        SetShaderValue(shader, timeLoc, &time, SHADER_UNIFORM_FLOAT);

        // Animation caméra (optionnel)
        float camX = sin(time * 0.5f) * 4.0f;
        float camZ = cos(time * 0.5f) * 4.0f;
        float camPosDyn[3] = { camX, 2.0f, camZ }; // Tourne autour
        
        // Envoi des positions
        SetShaderValue(shader, camPosLoc, camPosDyn, SHADER_UNIFORM_VEC3);
        
        float camTgt[3] = { camTarget.x, camTarget.y, camTarget.z };
        SetShaderValue(shader, camTargetLoc, camTgt, SHADER_UNIFORM_VEC3);

        BeginDrawing();
            ClearBackground(BLACK);
            BeginShaderMode(shader);
                DrawRectangle(0, 0, screenWidth, screenHeight, WHITE);
            EndShaderMode();
            
            DrawFPS(10, 10);
            DrawText("Path Tracing: GI & Soft Shadows", 10, 30, 20, WHITE);
        EndDrawing();
    }

    UnloadShader(shader);
    CloseWindow();

    return 0;
}