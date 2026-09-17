const std = @import("std");
const c = @cImport({
    @cInclude("SDL3/SDL.h");
});

pub fn main() !void {
    if (!c.SDL_Init(c.SDL_INIT_VIDEO)) {
        std.log.err("SDL_Init failed: {s}", .{c.SDL_GetError()});
        return error.SDLInitFailed;
    }
    defer c.SDL_Quit();

    var window: ?*c.SDL_Window = null;
    var renderer: ?*c.SDL_Renderer = null;

    if (!c.SDL_CreateWindowAndRenderer(
        "Swarm - SDL3 Window",
        1280, 720,
        0,
        &window,
        &renderer,
    )) {
        std.log.err("Failed to create window/renderer: {s}", .{c.SDL_GetError()});
        return error.SDLCreateWindowFailed;
    }
    defer c.SDL_DestroyRenderer(renderer);
    defer c.SDL_DestroyWindow(window);

    var running = true;
    var event: c.SDL_Event = undefined;

    while (running) {
        while (c.SDL_PollEvent(&event)) {
            switch (event.type) {
                c.SDL_EVENT_QUIT => {
                    running = false;
                },
                c.SDL_EVENT_KEY_DOWN => {
                    if (event.key.key == c.SDLK_ESCAPE) {
                        running = false;
                    }
                },
                else => {},
            }
        }

        _ = c.SDL_SetRenderDrawColor(renderer, 30, 32, 40, 255);
        _ = c.SDL_RenderClear(renderer);

        _ = c.SDL_RenderPresent(renderer);

        c.SDL_Delay(16);
    }
}
