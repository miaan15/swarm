package src

import "core:fmt"
import sdl "./vendor/sdl3"

import "./engine"
import "./engine/core"
import "./entity"
import "./game"
import "./global"

main_run :: proc() {
    global.init()

    if !sdl.Init({.VIDEO}) {
        fmt.eprintfln("SDL_Init Error: %s", sdl.GetError())
        return
    }
    defer sdl.Quit()

    if !sdl.CreateWindowAndRenderer("SDL3 global.window (Odin)", 1280, 720, {.RESIZABLE}, &global.window, &global.renderer) {
        fmt.eprintfln("Failed to create global.window/global.renderer: %s", sdl.GetError())
        return
    }
    defer sdl.DestroyRenderer(global.renderer)
    defer sdl.DestroyWindow(global.window)

    // INIT
    game.game_init()

    time_lastframe_s: f64 = 0
    running := true
    for running {
        event: sdl.Event
        for sdl.PollEvent(&event) {
            #partial switch event.type {
            case .QUIT:
                running = false
            case .KEY_DOWN:
                if event.key.key == sdl.K_ESCAPE {
                    running = false
                }
            }
        }

        // swap frame arena
        global.frame_arena_cur_idx = 1 - global.frame_arena_cur_idx
        global.frame_arena = &global.frame_arena_raw[global.frame_arena_cur_idx]
        core.arena_reset(global.frame_arena)

        // timer stuff
        global.time_s = f64(sdl.GetTicksNS()) / 1_000_000_000.0
        global.time_delta_s = global.time_s - time_lastframe_s
        time_lastframe_s = global.time_s

        global.tick_accumulate_time_s += global.time_delta_s

        // INPUT
        game.game_input()

        // tick stuff
        if global.time_delta_s > 0.2 { global.time_delta_s = 0.2 } // min = 5fps
        global.tick_accumulate_time_s += global.time_delta_s
        global.tick_delta_s = 1.0 / f64(global.tps)
        for global.tick_accumulate_time_s > global.tick_delta_s {
            global.tick_accumulate_time_s -= global.tick_delta_s
            global.tick_count_this_frame += 1

            global.tick_arena_cur_idx = 1 - global.tick_arena_cur_idx
            global.tick_arena = &global.tick_arena_raw[global.tick_arena_cur_idx]
            core.arena_reset(global.tick_arena)

            // UPDATE
            game.game_update()

            // systems
            entity.sprite_sys_update_early()
            entity.action_sys_update()
            entity.entity_sys_update()
            entity.collider_sys_update()

            // UPDATE LATE
            game.game_update_late()
        }
        global.tick_frame_alpha = global.tick_accumulate_time_s / global.tick_delta_s

        // VISUAL
        game.game_visual()

        // rendering
        sdl.SetRenderDrawColorFloat(global.renderer, 0, 0, 0, 0)
        sdl.RenderClear(global.renderer)

        // DRAW
        game.game_draw()

        // systems
        entity.sprite_sys_draw()
        engine.draw_present()
        // engine.draw_sys.len = 0

        sdl.RenderPresent(global.renderer)

        global.tick_count_this_frame = 0
    }
}
