package game

import "../engine"
import "../entity"

game_init :: proc() {
    engine.texture_sys_init(100)
    engine.draw_sys_init(1e5)

    engine.texture_load("img/char_base.png") // 1
    engine.texture_load("img/char_hair.png") // 2
}

game_input :: proc() { }
game_update :: proc() { }
game_update_late :: proc() { }
game_visual :: proc() { }
game_draw :: proc() { }
