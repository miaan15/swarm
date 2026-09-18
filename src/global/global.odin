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

OMNI_ARENA_SIZE :: #config(OMNI_ARENA_SIZE, 500 << 10 << 10)
TICK_ARENA_SIZE :: #config(TICK_ARENA_SIZE, 50 << 10 << 10)

// DIRECTORY
project_dir: string
src_dir:     string
asset_dir:   string

init :: proc() {
    // ARENA
    core.arena_init(&omni_arena, OMNI_ARENA_SIZE)

    core.arena_init(&tick_arena_raw[0], OMNI_ARENA_SIZE)
    core.arena_init(&tick_arena_raw[1], OMNI_ARENA_SIZE)

    tick_arena_cur_idx = 0;
    tick_arena = &tick_arena_raw[tick_arena_cur_idx];

    // DIRECTORY
    CURRENT_FILE :: #file

    SRC_DIR      := filepath.dir(filepath.dir(CURRENT_FILE))
    PROJECT_DIR  := filepath.dir(SRC_DIR)
    ASSET_DIR, _ := filepath.join({PROJECT_DIR, "asset"})

    src_dir     = SRC_DIR
    project_dir = PROJECT_DIR
    asset_dir   = ASSET_DIR
}
