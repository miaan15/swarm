#include <cstdio>
#include <SDL3/SDL.h>

import def;
import log;
import mem;

import context;
import context_game;

import draw;
import input;
import entity;

import game;

import benchmark;

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

    context_game_init();

    input_init();

    game_init();

    // benchmark stuff
    _benchmark_register("game:update");
    _benchmark_register("action:update");
    _benchmark_register("entity:update");
    _benchmark_register("collider:update");
    _benchmark_register("sprite:update");
    _benchmark_register("draw:sort");
    _benchmark_register("draw:render");
    _benchmark_enable = false;

    f64 time_last_frame_sec = 0; // for delta time
    bool running = true;

    while (running) {
        input_poll();

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: {
                    running = false;
                    break;
                }
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                    screen_width = event.window.data1;
                    screen_height = event.window.data2;
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

        // swap frame arena
        frame_arena_cur_idx = 1 - frame_arena_cur_idx;
        frame_arena_ptr = &frame_arena_raw[frame_arena_cur_idx];
        arena_reset(frame_arena_ptr);

        // timer
        time_sec = (f64)SDL_GetTicksNS() / 1'000'000'000;
        time_delta_sec = time_sec - time_last_frame_sec;
        time_last_frame_sec = time_sec;

        // clamp minimum fps to 5 fps
        if (time_delta_sec > 0.2) { time_delta_sec = 0.2; }

        tick_accumulated_time_sec += time_delta_sec;

        // Input
        game_input();

        // fixed tick update loop
        tick_delta_sec = 1.0 / static_cast<f64>(TPS);
        while (tick_accumulated_time_sec > tick_delta_sec) {
            tick_accumulated_time_sec -= tick_delta_sec;
            tick_count_this_frame += 1;

            // Swap tick arena
            tick_arena_cur_idx = 1 - tick_arena_cur_idx;
            tick_arena_ptr = &tick_arena_raw[tick_arena_cur_idx];
            arena_reset(tick_arena_ptr);

            //
            _benchmark_enable = true;

            // update
            _benchmark_start("game:update");
            game_update();
            _benchmark_end("game:update");

            // systems
            sprite_sys_update_early();

            _benchmark_start("action:update");
            action_sys_update();
            _benchmark_end("action:update");

            _benchmark_start("entity:update");
            entity_sys_update();
            _benchmark_end("entity:update");

            _benchmark_start("collider:update");
            collider_sys_update();
            _benchmark_end("collider:update");

            // update late
            game_update_late();

            input_cache_clear();
        }

        tick_frame_alpha = tick_accumulated_time_sec / tick_delta_sec;

        // visual
        game_visual();

        // rendering
        SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 0);
        SDL_RenderClear(renderer);

        // draw
        game_draw();

        // systems
        _benchmark_start("sprite:update");
        sprite_sys_draw();
        _benchmark_end("sprite:update");

        _benchmark_start("draw:sort");
        draw_sys_sort();
        _benchmark_end("draw:sort");

        _benchmark_start("draw:render");
        draw_sys_render();
        _benchmark_end("draw:render");

        draw_sys_reset();

        SDL_RenderPresent(renderer);

        tick_count_this_frame = 0;

        // benchmark update, reset
        _benchmark_update_registered();
        _benchmark_enable = false;
    }

    printf("%s\n", _benchmark_info_table_str(8, 20).c_str());

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
