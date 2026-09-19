package main

import "core:fmt"
import sdl "vendor:sdl3"

import "./global"
import "./engine"
import "./entity"

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

    // engine init
    engine.texture_sys_init(100)
    engine.draw_init(100)

    entity.entity_sys_init(1000)

    entity.entity_create({3, 3})
    entity.entity_create({2, 5})
    entity.entity_create({-2, 3})

    entity._chunk_mng_debug_log(&entity.entity_sys.entity_chunk)

    entity.entity_destroy(2)
    entity.entity_destroy(1)

    entity._chunk_mng_debug_log(&entity.entity_sys.entity_chunk)

    entity.entity_create({-2, -3})

    entity._chunk_mng_debug_log(&entity.entity_sys.entity_chunk)


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

        sdl.SetRenderDrawColorFloat(global.renderer, 0.1, 0.1, 0.1, 1.0)
        sdl.RenderClear(global.renderer)

        drw : ^engine.draw

        drw = engine.draw_make()
        drw.type = .BOX
        drw.box.rect = { 100, 100, 100, 100 }
        drw.box.color = { 50, 100, 255, 255 }

        drw = engine.draw_make()
        drw.type = .RECTANGLE
        drw.rectangle.rect = { 300, 300, 100, 100 }
        drw.rectangle.thickness = 10
        drw.rectangle.color = { 255, 100, 50, 255 }

        engine.draw_present()

        sdl.RenderPresent(global.renderer)
    }
}
