#include "engine.h"

#include <raylib.h>

int main(void)
{
    const int screenWidth = 1280;
    const int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "swarm");

    engine_init();

    while (!WindowShouldClose()) {
        engine_input();

        engine_update(); // FIXME this should be frame independence

        BeginDrawing();
            ClearBackground(RAYWHITE);
            engine_draw();
        EndDrawing();
    }

    CloseWindow();

    return 0;
}
