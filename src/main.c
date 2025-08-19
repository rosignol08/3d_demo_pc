/*******************************************************************************************
*
*   raylib [shaders] example - Apply a postprocessing shader to a scene
*
*   Example complexity rating: [★★★☆] 3/4
*
*   NOTE: This example requires raylib OpenGL 3.3 or ES2 versions for shaders support,
*         OpenGL 1.1 does not support shaders, recompile raylib to OpenGL 3.3 version.
*
*   NOTE: Shaders used in this example are #version 330 (OpenGL 3.3), to test this example
*         on OpenGL ES 2.0 platforms (Android, Raspberry Pi, HTML5), use #version 100 shaders
*         raylib comes with shaders ready for both versions, check raylib/shaders install folder
*
*   Example originally created with raylib 1.3, last time updated with raylib 4.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2015-2025 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"

#if defined(PLATFORM_DESKTOP)
    #define GLSL_VERSION            330
#else   // PLATFORM_ANDROID, PLATFORM_WEB
    #define GLSL_VERSION            100
#endif

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    SetConfigFlags(FLAG_MSAA_4X_HINT);      // Enable Multi Sampling Anti Aliasing 4x (if available)

    InitWindow(screenWidth, screenHeight, "jeu 3d");

    // Define the camera to look into our 3d world
    Camera camera = { 0 };
    camera.position = (Vector3){ 2.0f, 2.0f, 2.0f };    // Position de départ FPS
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };      // Regarde devant
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    // Mode FPS : masquer le curseur
    //DisableCursor();

    Shader shader = LoadShader(0,TextFormat( "vhs.fs"));

    // Create a RenderTexture2D to be used for render to texture
    RenderTexture2D target = LoadRenderTexture(screenWidth, screenHeight);

    // Génération procédurale du damier sans utiliser GenImageChecked
    int width = 256;
    int height = 256;
    int squareSize = 32;
    Color colors[2] = { BLACK, WHITE };

    Image damier = {
        .data = RL_MALLOC(width * height * 4),
        .width = width,
        .height = height,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };

    unsigned char *pixels = (unsigned char *)damier.data;
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int colorIndex = ((x / squareSize) + (y / squareSize)) % 2;
            Color c = colors[colorIndex];
            int idx = 4 * (y * width + x);
            pixels[idx + 0] = c.r;
            pixels[idx + 1] = c.g;
            pixels[idx + 2] = c.b;
            pixels[idx + 3] = c.a;
        }
    }

    Texture2D damierTexture = LoadTextureFromImage(damier);
    UnloadImage(damier);

    Mesh mesh = GenMeshPlane(100.0f, 100.0f, 100.0f, 100.0f); // Génère un mesh de plan

    Material material = LoadMaterialDefault(); // Matériau par défaut
    material.maps[MATERIAL_MAP_ALBEDO].texture = damierTexture; // Applique la texture damier

    Model model = LoadModelFromMesh(mesh); // Crée un modèle à partir du mesh
    model.materials[0] = material; // Associe le matériau au modèle

    Vector3 modelPosition = { 0.0f, 0.1f, 0.0f }; // Position du modèle dans la scène

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    // Get shader locations for passing custom parameters
    //int noiseTexLoc = GetShaderLocation(shaders[FX_VHS], "noise_texture");
    int timeParamLoc = GetShaderLocation(shader, "time");

    // The screen_texture is automatically bound to the fragment shader by raylib
    // We only need to add the time parameter which will be updated in the game loop
    float time = 0.0f;
    SetShaderValue(shader, timeParamLoc, &time, SHADER_UNIFORM_FLOAT);

    float resolution[2] = { (float)screenWidth, (float)screenHeight };

    float delta = GetFrameTime();
    // Main game loop
    while (!WindowShouldClose())        // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);
        //les controles
        if (IsKeyPressed(KEY_Q)){
            //tourne à gauche
            camera.position.x -= 0.1f * delta;
        }
        else if (IsKeyPressed(KEY_D)){
            //tourne à droite
            camera.position.x += 0.1f * delta;
        }
        else if (IsKeyPressed(KEY_Z)){
            //avance
            camera.position.z += 0.1f * delta;
        }
        else if (IsKeyPressed(KEY_S)){
            //recule
            camera.position.z -= 0.1f * delta;
        }

        // Draw
        //----------------------------------------------------------------------------------
        BeginTextureMode(target);       // Enable drawing to texture
            ClearBackground(RAYWHITE);  // Clear texture background

            BeginMode3D(camera);        // Begin 3d mode drawing
                //DrawModel(model, position, 0.1f, WHITE);   // Draw 3d model with texture
                //DrawMesh(cube,);
                DrawCube((Vector3){0.0f, 0.0f, 0.0f}, 1.0f, 1.0f, 1.0f, RED);
                //DrawPlane((Vector3){0.0f, 0.0f, 0.0f}, (Vector2){10.0f, 10.0f}, WHITE);
                DrawModel(model, modelPosition, 0.1f, WHITE);
                DrawGrid(10, 1.0f);     // Draw a grid
            EndMode3D();                // End 3d mode drawing, returns to orthographic 2d mode
        EndTextureMode();               // End drawing to texture (now we have a texture available for next passes)
        
        BeginDrawing();
            ClearBackground(RAYWHITE);  // Clear screen background
            
            time = GetTime();
            SetShaderValue(shader, timeParamLoc, &time, SHADER_UNIFORM_FLOAT);
            
            // Render generated texture using selected postprocessing shader
            BeginShaderMode(shader);
                // NOTE: Render texture must be y-flipped due to default OpenGL coordinates (left-bottom)
                DrawTextureRec(target.texture, (Rectangle){ 0, 0, (float)target.texture.width, (float)-target.texture.height }, (Vector2){ 0, 0 }, WHITE);
            EndShaderMode();
            DrawFPS(700, 15);
        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    // Unload all postpro shaders
    //UnloadTexture(texture);         // Unload texture
    //UnloadModel(model);             // Unload model
    UnloadRenderTexture(target);    // Unload render texture

    CloseWindow();                  // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
