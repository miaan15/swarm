package global

import "core:path/filepath"
import "../engine/core"
import sdl "vendor:sdl3"

// CONSTANT

// SDL
window: ^sdl.Window
renderer: ^sdl.Renderer

// AREMA
omni_arena: core.arena

tick_arena_raw: [2]core.arena
tick_arena_cur_idx: int
tick_arena: ^core.arena

frame_arena_raw: [2]core.arena
frame_arena_cur_idx: int
frame_arena: ^core.arena

OMNI_ARENA_SIZE :: #config(OMNI_ARENA_SIZE, 500 << 10 << 10)
TICK_ARENA_SIZE :: #config(TICK_ARENA_SIZE, 50 << 10 << 10)
FRAME_ARENA_SIZE :: #config(TICK_ARENA_SIZE, 50 << 10 << 10)

// DIRECTORY
project_dir: string
src_dir:     string
asset_dir:   string

// TIME
time_s, time_delta_s: f32
time_ms, time_delta_ms: u32

tick_total: f32
ticK_delta_s: f32
tick_delta_ms: u32
tick_frame_alpha: f32
tick_accumulate_time_ms: u32
tick_count_this_frame: u32

// CONFIG
tps: u32

init :: proc() {
    // ARENA
    core.arena_init(&omni_arena, OMNI_ARENA_SIZE)

    core.arena_init_in_arena(&tick_arena_raw[0], &omni_arena, TICK_ARENA_SIZE)
    core.arena_init_in_arena(&tick_arena_raw[1], &omni_arena, TICK_ARENA_SIZE)
    tick_arena_cur_idx = 0
    tick_arena = &tick_arena_raw[tick_arena_cur_idx]

    core.arena_init_in_arena(&frame_arena_raw[0], &omni_arena, FRAME_ARENA_SIZE)
    core.arena_init_in_arena(&frame_arena_raw[1], &omni_arena, FRAME_ARENA_SIZE)
    frame_arena_cur_idx = 0
    frame_arena = &frame_arena_raw[frame_arena_cur_idx]

    // DIRECTORY
    CURRENT_FILE :: #file

    src_dir      = filepath.dir(filepath.dir(CURRENT_FILE))
    project_dir  = filepath.dir(src_dir)
    asset_dir, _ = filepath.join({project_dir, "asset"})
    // core.log_info("%s\n%s\n%s", src_dir, project_dir, asset_dir)

    // CONFIG
    tps = 5
}
