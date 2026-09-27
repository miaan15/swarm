// get input from keyboard (just keyboard for now)
// - API pretty similar to classic input input
// - tick cache helper: input supposed to just appear in frame update, but this help cache to the next tick

module;

#include <cassert>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_scancode.h>

export module input;

import def;

import context;

export namespace sw {

enum struct SCANCODE {
    NIL = SDL_SCANCODE_UNKNOWN,

    // letters
    A = SDL_SCANCODE_A,
    B = SDL_SCANCODE_B,
    C = SDL_SCANCODE_C,
    D = SDL_SCANCODE_D,
    E = SDL_SCANCODE_E,
    F = SDL_SCANCODE_F,
    G = SDL_SCANCODE_G,
    H = SDL_SCANCODE_H,
    I = SDL_SCANCODE_I,
    J = SDL_SCANCODE_J,
    K = SDL_SCANCODE_K,
    L = SDL_SCANCODE_L,
    M = SDL_SCANCODE_M,
    N = SDL_SCANCODE_N,
    O = SDL_SCANCODE_O,
    P = SDL_SCANCODE_P,
    Q = SDL_SCANCODE_Q,
    R = SDL_SCANCODE_R,
    S = SDL_SCANCODE_S,
    T = SDL_SCANCODE_T,
    U = SDL_SCANCODE_U,
    V = SDL_SCANCODE_V,
    W = SDL_SCANCODE_W,
    X = SDL_SCANCODE_X,
    Y = SDL_SCANCODE_Y,
    Z = SDL_SCANCODE_Z,

    // number row
    NUM_1 = SDL_SCANCODE_1,
    NUM_2 = SDL_SCANCODE_2,
    NUM_3 = SDL_SCANCODE_3,
    NUM_4 = SDL_SCANCODE_4,
    NUM_5 = SDL_SCANCODE_5,
    NUM_6 = SDL_SCANCODE_6,
    NUM_7 = SDL_SCANCODE_7,
    NUM_8 = SDL_SCANCODE_8,
    NUM_9 = SDL_SCANCODE_9,
    NUM_0 = SDL_SCANCODE_0,

    // control keys
    RETURN = SDL_SCANCODE_RETURN,
    ESCAPE = SDL_SCANCODE_ESCAPE,
    BACKSPACE = SDL_SCANCODE_BACKSPACE,
    TAB = SDL_SCANCODE_TAB,
    SPACE = SDL_SCANCODE_SPACE,
    CAPSLOCK = SDL_SCANCODE_CAPSLOCK,
    INSERT = SDL_SCANCODE_INSERT,
    HOME = SDL_SCANCODE_HOME,
    PAGEUP = SDL_SCANCODE_PAGEUP,
    DELETE = SDL_SCANCODE_DELETE,
    END = SDL_SCANCODE_END,
    PAGEDOWN = SDL_SCANCODE_PAGEDOWN,
    PRINTSCREEN = SDL_SCANCODE_PRINTSCREEN,
    SCROLLLOCK = SDL_SCANCODE_SCROLLLOCK,
    PAUSE = SDL_SCANCODE_PAUSE,

    // arrow keys
    RIGHT = SDL_SCANCODE_RIGHT,
    LEFT = SDL_SCANCODE_LEFT,
    DOWN = SDL_SCANCODE_DOWN,
    UP = SDL_SCANCODE_UP,

    // function keys
    F1 = SDL_SCANCODE_F1,
    F2 = SDL_SCANCODE_F2,
    F3 = SDL_SCANCODE_F3,
    F4 = SDL_SCANCODE_F4,
    F5 = SDL_SCANCODE_F5,
    F6 = SDL_SCANCODE_F6,
    F7 = SDL_SCANCODE_F7,
    F8 = SDL_SCANCODE_F8,
    F9 = SDL_SCANCODE_F9,
    F10 = SDL_SCANCODE_F10,
    F11 = SDL_SCANCODE_F11,
    F12 = SDL_SCANCODE_F12,

    // punctuation / symbols
    MINUS = SDL_SCANCODE_MINUS,
    EQUALS = SDL_SCANCODE_EQUALS,
    LEFTBRACKET = SDL_SCANCODE_LEFTBRACKET,
    RIGHTBRACKET = SDL_SCANCODE_RIGHTBRACKET,
    BACKSLASH = SDL_SCANCODE_BACKSLASH,
    SEMICOLON = SDL_SCANCODE_SEMICOLON,
    APOSTROPHE = SDL_SCANCODE_APOSTROPHE,
    GRAVE = SDL_SCANCODE_GRAVE,
    COMMA = SDL_SCANCODE_COMMA,
    PERIOD = SDL_SCANCODE_PERIOD,
    SLASH = SDL_SCANCODE_SLASH,

    // numpad
    NUMLOCKCLEAR = SDL_SCANCODE_NUMLOCKCLEAR,
    KP_DIVIDE = SDL_SCANCODE_KP_DIVIDE,
    KP_MULTIPLY = SDL_SCANCODE_KP_MULTIPLY,
    KP_MINUS = SDL_SCANCODE_KP_MINUS,
    KP_PLUS = SDL_SCANCODE_KP_PLUS,
    KP_ENTER = SDL_SCANCODE_KP_ENTER,
    KP_1 = SDL_SCANCODE_KP_1,
    KP_2 = SDL_SCANCODE_KP_2,
    KP_3 = SDL_SCANCODE_KP_3,
    KP_4 = SDL_SCANCODE_KP_4,
    KP_5 = SDL_SCANCODE_KP_5,
    KP_6 = SDL_SCANCODE_KP_6,
    KP_7 = SDL_SCANCODE_KP_7,
    KP_8 = SDL_SCANCODE_KP_8,
    KP_9 = SDL_SCANCODE_KP_9,
    KP_0 = SDL_SCANCODE_KP_0,
    KP_PERIOD = SDL_SCANCODE_KP_PERIOD,
    KP_EQUALS = SDL_SCANCODE_KP_EQUALS,

    // modifier keys
    LCTRL = SDL_SCANCODE_LCTRL,
    LSHIFT = SDL_SCANCODE_LSHIFT,
    LALT = SDL_SCANCODE_LALT,
    LGUI = SDL_SCANCODE_LGUI,
    RCTRL = SDL_SCANCODE_RCTRL,
    RSHIFT = SDL_SCANCODE_RSHIFT,
    RALT = SDL_SCANCODE_RALT,
    RGUI = SDL_SCANCODE_RGUI,
};

struct {
    const bool *cur_keyboard_state;
    bool *last_poll_keyboard_state;
    u8 *cache_keyboard_state;
    // cache keyboard state is masked as 3bits: bit-0: down; bit-1: up; bit-2: on
    // but the way this cache - every poll just add more to cache, not overwrite, make the mask not really straight forward
    int numkeys;
} input_sys = {};

void input_init() {
    input_sys.cur_keyboard_state = SDL_GetKeyboardState(&input_sys.numkeys);
    input_sys.last_poll_keyboard_state = (bool*)arena_alloc(&omni_arena, input_sys.numkeys * sizeof(bool));
    input_sys.cache_keyboard_state = (u8*)arena_alloc(&omni_arena, input_sys.numkeys * sizeof(u8));
}

bool input_is_key_down(SCANCODE scancode) {
    return input_sys.cur_keyboard_state[(usize)scancode]
           && !input_sys.last_poll_keyboard_state[(usize)scancode];
}

bool input_is_key_up(SCANCODE scancode) {
    return !input_sys.cur_keyboard_state[(usize)scancode]
           && input_sys.last_poll_keyboard_state[(usize)scancode];
}

bool input_is_key_on(SCANCODE scancode) {
    return input_sys.cur_keyboard_state[(usize)scancode];
}

bool input_is_key_down_cache(SCANCODE scancode) {
    return (input_sys.cache_keyboard_state[(usize)scancode] >> 0) & 1;
}

bool input_is_key_up_cache(SCANCODE scancode) {
    // only on edge case where "down" and "up" before a tick
    return ((input_sys.cache_keyboard_state[(usize)scancode] >> 1) & 1) &
           (input_sys.cache_keyboard_state[(usize)scancode] != 0b111);
}

bool input_is_key_on_cache(SCANCODE scancode) {
    // if "up" before the tick, "on"-bit still on, but it should be not "on"
    // also 1 edge case
    return (((input_sys.cache_keyboard_state[(usize)scancode] >> 2) & 1) &
            !((input_sys.cache_keyboard_state[(usize)scancode] >> 1) & 1))
           | (input_sys.cache_keyboard_state[(usize)scancode] == 0b111);
}

void input_poll() {
    // this need to be called before the SDL_PollEvent

    assert(input_sys.cur_keyboard_state);
    assert(input_sys.last_poll_keyboard_state);

    // eval cache, before update last keyboard state
    for (usize i = 0; i < input_sys.numkeys; ++i) {
        bool cur = input_sys.cur_keyboard_state[i];
        bool prev = input_sys.last_poll_keyboard_state[i];

        bool is_down = cur && !prev;
        bool is_up   = !cur && prev;
        bool is_on   = cur;

        input_sys.cache_keyboard_state[i] |= (is_down << 0) |
                                             (is_up   << 1) |
                                             (is_on   << 2);
    }

    // update last_keyboard_state
    memcpy(input_sys.last_poll_keyboard_state, input_sys.cur_keyboard_state, input_sys.numkeys * sizeof(bool));
}

void input_cache_clear() {
    for (usize i = 0; i < input_sys.numkeys; ++i) {
        // if an key is "down" and "up" before a tick => the "up" state delay for 1 tick
        input_sys.cache_keyboard_state[i] = input_sys.cache_keyboard_state[i] == 0b111 ? 0b010 : 0;
    }
}

}
