// context but to avoid circle import

module;

export module context_game;

import draw;

import context;

export namespace sw {

camera main_camera;

void context_game_init() {
    main_camera = { {0, 0}, screen_width };
}

}
