#include <cstdio>
#include <SDL3/SDL.h>

import def;
import mem;
import context;

using namespace sw;

int main(int argc, char *argv[]) {
    context_init();

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("SDL3 global.window (C++)", 1280, 720, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        std::fprintf(stderr, "Failed to create window/renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // game_init();

    f64 time_last_frame_sec = 0.0;
    bool running = true;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_KEY_DOWN: {
                    if (event.key.key == SDLK_ESCAPE) {
                        running = false;
                    }
                    break;
                }
                default:
                    break;
            }
        }

        // Swap frame arena
        frame_arena_cur_idx = 1 - frame_arena_cur_idx;
        frame_arena_ptr = &frame_arena_raw[frame_arena_cur_idx];
        arena_reset(frame_arena_ptr);

        // Timer
        time_sec = static_cast<f64>(SDL_GetTicksNS()) / 1'000'000'000.0;
        time_delta_sec = time_sec - time_last_frame_sec;
        time_last_frame_sec = time_sec;

        // Clamp minimum fps to 5 FPS
        if (time_delta_sec > 0.2) {
            time_delta_sec = 0.2;
        }

        tick_accumulated_time_sec += time_delta_sec;

        // Input
        // game_input();

        // Fixed tick update loop
        tick_delta_sec = 1.0 / static_cast<f64>(TPS);
        while (tick_accumulated_time_sec > tick_delta_sec) {
            tick_accumulated_time_sec -= tick_delta_sec;
            tick_count_this_frame += 1;

            // Swap tick arena
            tick_arena_cur_idx = 1 - tick_arena_cur_idx;
            tick_arena_ptr = &tick_arena_raw[tick_arena_cur_idx];
            arena_reset(tick_arena_ptr);

            // Update
            // game_update();

            // Systems
            // sprite_sys_update_early();
            // action_sys_update();
            // entity_sys_update();
            // collider_sys_update();

            // Update late
            // game_update_late();
        }

        tick_frame_alpha = tick_accumulated_time_sec / tick_delta_sec;

        // Visual
        // game_visual();

        // Rendering
        SDL_SetRenderDrawColorFloat(renderer, 0.0f, 0.0f, 0.0f, 0.0f);
        SDL_RenderClear(renderer);

        // Draw
        // game_draw();

        // Systems
        // sprite_sys_draw();
        // draw_present();

        SDL_RenderPresent(renderer);

        tick_count_this_frame = 0;
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
