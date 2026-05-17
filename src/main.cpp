#include <SDL.h>
#include <SDL_mixer.h>
#include <SDL_ttf.h>
#include <windows.h>
#include <stdio.h>

#include "include/common.h"
#include "include/textures.h"
#include "include/input.h"
#include "include/world.h"
#include "include/generator.h"
#include "include/player.h"
#include "include/entity.h"
#include "include/audio.h"
#include "include/renderer.h"
#include "include/hud.h"
#include "include/savegame.h"
#include "include/menu.h"

static GameState g_state      = GS_MENU;
static bool      g_running    = true;
static bool      g_fullscreen = false;
static double    g_fps        = 0.0;
static double    g_frametime  = 0.0;
static bool      g_mouse_captured = false;
static SDL_Window*   g_window   = nullptr;
static SDL_Renderer* g_renderer = nullptr;

static void mouse_capture(bool capture) {
    g_mouse_captured = capture;
    SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE);
    SDL_ShowCursor(capture ? SDL_DISABLE : SDL_ENABLE);
}

static void toggle_fullscreen() {
    g_fullscreen = !g_fullscreen;
    if (g_fullscreen)
        SDL_SetWindowFullscreen(g_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    else
        SDL_SetWindowFullscreen(g_window, 0);
}

static TTF_Font* load_font(int size) {
    const char* paths[] = {
        "C:\\Windows\\Fonts\\cour.ttf",
        "C:\\Windows\\Fonts\\courbd.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\arialbd.ttf",
        "C:\\Windows\\Fonts\\verdana.ttf",
        "C:\\Windows\\Fonts\\tahoma.ttf",
        nullptr
    };
    for (int i=0; paths[i]; i++) {
        TTF_Font* f = TTF_OpenFont(paths[i], size);
        if (f) return f;
    }
    return nullptr;
}

static void game_start_level(int level) {
    world_generate(level);
    entities_spawn_for_level(level, g_world.seed);
    player_init(g_player, g_world.playerStart, level);
    world_update_room_states(g_player.pos);
    audio_play_music(level);
    g_state = GS_PLAYING;
}

static void game_new() {
    game_start_level(1);
}

static void game_next_level() {
    int next    = std::min(g_player.level + 1, MAX_LEVELS);
    int score   = g_player.score;
    int kills   = g_player.kills;
    Weapon weapons[MAX_WEAPONS];
    for (int i=0;i<MAX_WEAPONS;i++) weapons[i] = g_player.weapons[i];
    int curWpn  = g_player.currentWeapon;

    game_start_level(next);

    g_player.score = score;
    g_player.kills = kills;
    for (int i=0;i<MAX_WEAPONS;i++) g_player.weapons[i] = weapons[i];
    g_player.currentWeapon = curWpn;
}

static void handle_shoot() {
    if (!player_can_fire(g_player)) return;
    if (!player_do_fire(g_player))  return;

    audio_play(weapon_sound(g_player.currentWeapon));

    int    wpn     = g_player.currentWeapon;
    int    dmg     = weapon_damage[wpn];
    double sp      = weapon_spread[wpn];
    int    pellets = (wpn==(int)WPN_SHOTGUN)       ? 7  :
                     (wpn==(int)WPN_SUPER_SHOTGUN) ? 14 : 1;

    for (int pel=0; pel<pellets; pel++) {
        double angle = g_player.angle;
        if (sp > 0) {
            double r = ((double)(rand()%10000)/10000.0 - 0.5)*2.0*sp;
            angle += r;
        }
        Vec2 dir = {cos(angle), sin(angle)};
        int pelDmg = pellets>1 ? dmg/pellets : dmg;

        if (wpn==(int)WPN_CHAINSAW || wpn==(int)WPN_FISTS) {
            for (int i=0;i<g_entityCount;i++) {
                Entity& e=g_entities[i];
                if (!e.active||e.type!=ENT_ENEMY||e.state==ES_DEAD) continue;
                if (e.pos.distTo(g_player.pos)<1.5)
                    entity_take_damage(i, dmg, g_player);
            }
        } else if (wpn==(int)WPN_BFG) {
            for (int i=0;i<g_entityCount;i++) {
                Entity& e=g_entities[i];
                if (!e.active||e.type!=ENT_ENEMY||e.state==ES_DEAD) continue;
                if (e.inAggro) entity_take_damage(i, dmg, g_player);
            }
        } else {
            EntityHitResult hit = entities_raycast_hit(g_player.pos, dir, 30.0, pelDmg);
            if (hit.hit) {
                entity_take_damage(hit.entityIdx, hit.damage, g_player);
                audio_play_3d(SND_ENEMY_DEATH, g_entities[hit.entityIdx].pos, g_player.pos);
            }
        }
    }
}

static void update_game(double dt) {
    bool footstep = false;
    player_update(g_player, dt, footstep);
    if (footstep) audio_play(step_sound(g_player.pos), 0.4);

    handle_shoot();

    bool playerHit = entities_update(dt, g_player);
    if (playerHit) audio_play(SND_PLAYER_HURT);

    world_update_doors(dt);
    world_update_room_states(g_player.pos);
    world_update_lights(dt);

    if (!g_player.alive) {
        audio_play(SND_PLAYER_DEATH);
        g_state = GS_DEAD;
        mouse_capture(false);
        return;
    }

    int ex=(int)g_world.exitPos.x, ey=(int)g_world.exitPos.y;
    if ((int)g_player.pos.x==ex && (int)g_player.pos.y==ey) {
        audio_play(SND_LEVEL_COMPLETE);
        savegame_autosave(g_player);
        g_state = GS_LEVEL_COMPLETE;
        mouse_capture(false);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) return 1;

    g_window = SDL_CreateWindow(
        NZ_TITLE " v" NZ_VERSION,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H,
        SDL_WINDOW_SHOWN|SDL_WINDOW_INPUT_FOCUS);
    if (!g_window) { SDL_Quit(); return 1; }

    g_renderer = SDL_CreateRenderer(g_window, -1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!g_renderer) { SDL_DestroyWindow(g_window); SDL_Quit(); return 1; }

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    SDL_RaiseWindow(g_window);

    hud_init();
    g_font_large  = load_font(28);
    g_font_medium = load_font(20);
    g_font_small  = load_font(14);

    textures_init();
    renderer_init(g_renderer);
    audio_init();
    input_init();
    savegame_init_dirs();
    savegame_load_registry();
    menu_init();

    mouse_capture(false);

    Uint64 perfFreq    = SDL_GetPerformanceFrequency();
    Uint64 prevTime    = SDL_GetPerformanceCounter();
    double accumulator = 0.0;
    double fixedDt     = 1.0 / FIXED_TICKS;
    int    frameCount  = 0;
    double fpsTimer    = 0.0;

    while (g_running && !g_input.quit) {
        Uint64 now   = SDL_GetPerformanceCounter();
        double delta = (double)(now - prevTime) / (double)perfFreq;
        prevTime     = now;
        if (delta > 0.1) delta = 0.1;
        g_frametime  = delta;
        fpsTimer    += delta;
        frameCount++;
        if (fpsTimer >= 0.5) {
            g_fps      = (double)frameCount / fpsTimer;
            frameCount = 0;
            fpsTimer   = 0.0;
        }

        input_begin_frame();
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) { g_running = false; break; }
            if (ev.type == SDL_WINDOWEVENT) {
                if (ev.window.event == SDL_WINDOWEVENT_FOCUS_LOST)
                    mouse_capture(false);
                if (ev.window.event == SDL_WINDOWEVENT_FOCUS_GAINED && g_state==GS_PLAYING)
                    mouse_capture(true);
            }
            input_process(ev);
        }

        if (key_pressed(SDL_SCANCODE_F5)) toggle_fullscreen();
        if (action_debug()) hud_toggle_debug();
        if (action_map())   hud_toggle_map();

        switch (g_state) {
        case GS_MENU:
        case GS_PAUSED: {
            MenuAction act = menu_update(g_player, g_state);
            switch (act) {
            case MACT_NEW_GAME:
                game_new();
                mouse_capture(true);
                break;
            case MACT_CONTINUE:
            case MACT_LOAD_OK:
                world_generate(g_player.level);
                entities_spawn_for_level(g_player.level, g_world.seed);
                world_update_room_states(g_player.pos);
                audio_play_music(g_player.level);
                g_state = GS_PLAYING;
                mouse_capture(true);
                break;
            case MACT_RESUME:
                g_state = GS_PLAYING;
                mouse_capture(true);
                break;
            case MACT_GOTO_MAIN:
                g_state = GS_MENU;
                mouse_capture(false);
                break;
            case MACT_QUIT:
                g_running = false;
                break;
            default: break;
            }
            break;
        }
        case GS_PLAYING: {
            if (action_pause()) {
                menu_open_pause(GS_PLAYING);
                g_state = GS_PAUSED;
                mouse_capture(false);
                break;
            }
            accumulator += delta;
            while (accumulator >= fixedDt) {
                update_game(fixedDt);
                accumulator -= fixedDt;
                if (g_state != GS_PLAYING) { accumulator=0; break; }
            }
            break;
        }
        case GS_DEAD: {
            if (key_pressed(SDL_SCANCODE_R)) {
                game_new();
                mouse_capture(true);
            }
            if (action_pause()) {
                g_state = GS_MENU;
                menu_open_main();
                mouse_capture(false);
            }
            break;
        }
        case GS_LEVEL_COMPLETE: {
            if (key_pressed(SDL_SCANCODE_RETURN)||key_pressed(SDL_SCANCODE_KP_ENTER)) {
                game_next_level();
                mouse_capture(true);
            }
            break;
        }
        default: break;
        }

        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 255);
        SDL_RenderClear(g_renderer);

        if (g_state==GS_PLAYING||g_state==GS_DEAD||g_state==GS_LEVEL_COMPLETE) {
            renderer_draw_frame(g_renderer, g_player);
            hud_draw(g_renderer, g_player, g_fps, g_frametime, g_state);
        }

        if (g_state==GS_MENU||g_state==GS_PAUSED) {
            if (g_state==GS_PAUSED) {
                renderer_draw_frame(g_renderer, g_player);
                SDL_SetRenderDrawColor(g_renderer,0,0,0,140);
                SDL_SetRenderDrawBlendMode(g_renderer,SDL_BLENDMODE_BLEND);
                SDL_Rect full={0,0,SCREEN_W,SCREEN_H};
                SDL_RenderFillRect(g_renderer,&full);
            }
            menu_render(g_renderer, delta);
        }

        SDL_RenderPresent(g_renderer);
    }

    mouse_capture(false);
    audio_shutdown();
    renderer_destroy();
    hud_shutdown();
    SDL_DestroyRenderer(g_renderer);
    SDL_DestroyWindow(g_window);
    SDL_Quit();
    return 0;
}
