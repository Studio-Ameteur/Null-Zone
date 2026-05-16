#pragma once
#include <SDL.h>
#include "common.h"

struct InputState {
    bool keys[512];
    bool keysPrev[512];
    bool mouseLeft;
    bool mouseRight;
    bool mouseLeftPrev;
    bool mouseRightPrev;
    int  mouseRelX;
    int  mouseRelY;
    int  mouseWheel;
    bool quit;
};

static InputState g_input = {};

inline void input_init() {
    memset(&g_input, 0, sizeof(g_input));
}

inline void input_begin_frame() {
    memcpy(g_input.keysPrev,    g_input.keys, sizeof(g_input.keys));
    g_input.mouseLeftPrev  = g_input.mouseLeft;
    g_input.mouseRightPrev = g_input.mouseRight;
    g_input.mouseRelX      = 0;
    g_input.mouseRelY      = 0;
    g_input.mouseWheel     = 0;
}

inline bool input_process(SDL_Event& e) {
    if (e.type == SDL_QUIT) {
        g_input.quit = true;
        return true;
    }
    if (e.type == SDL_KEYDOWN && e.key.keysym.scancode < 512)
        g_input.keys[e.key.keysym.scancode] = true;
    if (e.type == SDL_KEYUP && e.key.keysym.scancode < 512)
        g_input.keys[e.key.keysym.scancode] = false;
    if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_LEFT)  g_input.mouseLeft  = true;
        if (e.button.button == SDL_BUTTON_RIGHT) g_input.mouseRight = true;
    }
    if (e.type == SDL_MOUSEBUTTONUP) {
        if (e.button.button == SDL_BUTTON_LEFT)  g_input.mouseLeft  = false;
        if (e.button.button == SDL_BUTTON_RIGHT) g_input.mouseRight = false;
    }
    if (e.type == SDL_MOUSEMOTION) {
        g_input.mouseRelX += e.motion.xrel;
        g_input.mouseRelY += e.motion.yrel;
    }
    if (e.type == SDL_MOUSEWHEEL)
        g_input.mouseWheel = e.wheel.y;
    return false;
}

inline bool key_held(SDL_Scancode sc)     { return g_input.keys[sc]; }
inline bool key_pressed(SDL_Scancode sc)  { return  g_input.keys[sc] && !g_input.keysPrev[sc]; }
inline bool key_released(SDL_Scancode sc) { return !g_input.keys[sc] &&  g_input.keysPrev[sc]; }

inline bool move_forward()    { return key_held(SDL_SCANCODE_W) || key_held(SDL_SCANCODE_UP);    }
inline bool move_backward()   { return key_held(SDL_SCANCODE_S) || key_held(SDL_SCANCODE_DOWN);  }
inline bool move_left()       { return key_held(SDL_SCANCODE_A) || key_held(SDL_SCANCODE_LEFT);  }
inline bool move_right()      { return key_held(SDL_SCANCODE_D) || key_held(SDL_SCANCODE_RIGHT); }
inline bool action_use()      { return key_pressed(SDL_SCANCODE_E);                              }
inline bool action_map()      { return key_pressed(SDL_SCANCODE_TAB);                            }
inline bool action_debug()    { return key_pressed(SDL_SCANCODE_F3);                             }
inline bool action_pause()    { return key_pressed(SDL_SCANCODE_ESCAPE);                         }
inline bool action_wpnhud()   { return key_held(SDL_SCANCODE_Z);                                 }
inline bool fire()            { return g_input.mouseLeft;                                        }
inline bool fire_pressed()    { return g_input.mouseLeft && !g_input.mouseLeftPrev;              }
inline int  wheel()           { return g_input.mouseWheel;                                       }
inline int  mouse_dx()        { return g_input.mouseRelX;                                        }

inline bool weapon_key(int slot) {
    static const SDL_Scancode wkeys[9] = {
        SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3,
        SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6,
        SDL_SCANCODE_7, SDL_SCANCODE_8, SDL_SCANCODE_9
    };
    if (slot < 0 || slot >= 9) return false;
    return key_pressed(wkeys[slot]);
}
