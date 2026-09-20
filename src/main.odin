package main

import "core:fmt"
import sdl "vendor:sdl3"

import "./engine"
import "./engine/core"
import "./entity"
import "./game"
import "./global"

main :: proc() {
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

    time_lastframe_ms: u32 = 0
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
        global.time_ms = u32(sdl.GetTicks())
        global.time_delta_ms = global.time_ms - time_lastframe_ms
        time_lastframe_ms = global.time_ms
        global.time_s = f32(global.time_ms) / 1000
        global.time_delta_s = f32(global.time_delta_ms) / 1000

        global.tick_accumulate_time_ms += global.time_delta_ms

        // INPUT
        game.game_input()

        // tick stuff
        global.tick_delta_ms = 1000 / global.tps
        if global.tick_delta_ms < global.time_delta_ms { global.tick_delta_ms = global.time_delta_ms }
        if global.tick_delta_ms > 1000 / 5 { global.tick_delta_ms = 1000 / 5 }
        global.ticK_delta_s = f32(global.tick_delta_ms) / 1000
        for global.tick_accumulate_time_ms > global.tick_delta_ms {
            global.tick_accumulate_time_ms -= global.tick_delta_ms
            global.tick_count_this_frame += 1

            global.tick_arena_cur_idx = 1 - global.tick_arena_cur_idx
            global.tick_arena = &global.tick_arena_raw[global.tick_arena_cur_idx]
            core.arena_reset(global.tick_arena)

            // UPDATE
            game.game_update()

            // systems
            entity.action_sys_update()
            entity.entity_sys_update()
            entity.sprite_sys_update()
            entity.collider_sys_update()

            // UPDATE LATE
            game.game_update_late()
        }
        global.tick_frame_alpha = f32(global.tick_accumulate_time_ms) / f32(global.tick_delta_ms)

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

        sdl.RenderPresent(global.renderer)
    }
}
