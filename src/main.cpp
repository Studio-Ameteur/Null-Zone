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

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#endif

static GameState  g_state      = GS_MENU;
static bool       g_running    = true;
static double     g_fps        = 0.0;
static double     g_frametime  = 0.0;
static bool       g_footstep   = false;

static TTF_Font* load_font_fallback(int size) {
    const char* paths[] = {
        "C:\\Windows\\Fonts\\cour.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
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
    int next = g_player.level + 1;
    if (next > MAX_LEVELS) next = MAX_LEVELS;
    int score  = g_player.score;
    int kills  = g_player.kills;
    Weapon weapons[MAX_WEAPONS];
    for (int i=0;i<MAX_WEAPONS;i++) weapons[i]=g_player.weapons[i];
    int curWpn = g_player.currentWeapon;

    game_start_level(next);

    g_player.score = score;
    g_player.kills = kills;
    for (int i=0;i<MAX_WEAPONS;i++) g_player.weapons[i]=weapons[i];
    g_player.currentWeapon = curWpn;
}

static void handle_shoot() {
    if (!player_can_fire(g_player)) return;
    if (!player_do_fire(g_player))  return;

    audio_play(weapon_sound(g_player.currentWeapon));

    int wpn   = g_player.currentWeapon;
    int dmg   = weapon_damage[wpn];
    double sp = weapon_spread[wpn];

    int pellets = (wpn==(int)WPN_SHOTGUN)      ? 7  :
                  (wpn==(int)WPN_SUPER_SHOTGUN) ? 14 : 1;

    for (int pel=0; pel<pellets; pel++) {
        double angle = g_player.angle;
        if (sp > 0) {
            double r = ((double)(rand()%10000)/10000.0 - 0.5) * 2.0 * sp;
            angle += r;
        }
        Vec2 dir = {cos(angle), sin(angle)};
        int pelDmg = (pellets>1) ? dmg/pellets : dmg;

        if (wpn==(int)WPN_CHAINSAW || wpn==(int)WPN_FISTS) {
            for (int i=0;i<g_entityCount;i++) {
                Entity& e=g_entities[i];
                if (!e.active||e.type!=ENT_ENEMY||e.state==ES_DEAD) continue;
                double dist=e.pos.distTo(g_player.pos);
                if (dist<1.5) entity_take_damage(i, dmg, g_player);
            }
        } else if (wpn==(int)WPN_BFG) {
            for (int i=0;i<g_entityCount;i++) {
                Entity& e=g_entities[i];
                if (!e.active||e.type!=ENT_ENEMY||e.state==ES_DEAD) continue;
                if (e.inAggro) entity_take_damage(i, dmg, g_player);
            }
        } else {
            EntityHitResult hit = entities_raycast_hit(
                g_player.pos, dir, 30.0, pelDmg);
            if (hit.hit) {
                entity_take_damage(hit.entityIdx, hit.damage, g_player);
                audio_play_3d(SND_ENEMY_DEATH,
                    g_entities[hit.entityIdx].pos, g_player.pos);
            }
        }
    }
}

static void update_game(double dt) {
    input_begin_frame();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (input_process(e)) { g_running=false; return; }
    }

    if (action_debug()) hud_toggle_debug();
    if (action_map())   hud_toggle_map();

    if (action_pause()) {
        menu_open_pause(GS_PLAYING);
        g_state = GS_PAUSED;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        return;
    }

    player_update(g_player, dt, g_footstep);

    if (g_footstep) {
        SoundID ss = step_sound(g_player.pos);
        audio_play(ss, 0.4);
    }

    handle_shoot();

    bool playerHit = entities_update(dt, g_player);
    if (playerHit) audio_play(SND_PLAYER_HURT);

    world_update_doors(dt);
    world_update_room_states(g_player.pos);
    world_update_lights(dt);

    if (!g_player.alive) {
        audio_play(SND_PLAYER_DEATH);
        g_state = GS_DEAD;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        return;
    }

    int ex=(int)g_world.exitPos.x, ey=(int)g_world.exitPos.y;
    if ((int)g_player.pos.x==ex && (int)g_player.pos.y==ey) {
        audio_play(SND_LEVEL_COMPLETE);
        savegame_autosave(g_player);
        g_state = GS_LEVEL_COMPLETE;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) < 0) return 1;

    SDL_Window* window = SDL_CreateWindow(
        NZ_TITLE " v" NZ_VERSION,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    if (!window) { SDL_Quit(); return 1; }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1,
        SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) { SDL_DestroyWindow(window); SDL_Quit(); return 1; }

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRelativeMouseMode(SDL_FALSE);

    hud_init();
    g_font_large  = load_font_fallback(28);
    g_font_medium = load_font_fallback(20);
    g_font_small  = load_font_fallback(14);

    textures_init();
    renderer_init(renderer);
    audio_init();
    input_init();
    savegame_init_dirs();
    savegame_load_registry();
    menu_init();

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
        if (delta > 0.05) delta = 0.05;
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
            if (input_process(ev)) g_running=false;
        }

        switch(g_state) {
        case GS_MENU:
        case GS_PAUSED: {
            MenuAction act = menu_update(g_player, g_state);
            switch(act) {
            case MACT_NEW_GAME:
                SDL_SetRelativeMouseMode(SDL_TRUE);
                game_new();
                break;
            case MACT_CONTINUE:
            case MACT_LOAD_OK:
                world_generate(g_player.level);
                entities_spawn_for_level(g_player.level, g_world.seed);
                world_update_room_states(g_player.pos);
                audio_play_music(g_player.level);
                SDL_SetRelativeMouseMode(SDL_TRUE);
                g_state=GS_PLAYING;
                break;
            case MACT_RESUME:
                SDL_SetRelativeMouseMode(SDL_TRUE);
                g_state=GS_PLAYING;
                break;
            case MACT_GOTO_MAIN:
                SDL_SetRelativeMouseMode(SDL_FALSE);
                g_state=GS_MENU;
                break;
            case MACT_QUIT:
                g_running=false;
                break;
            default: break;
            }
            break;
        }
        case GS_PLAYING: {
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
                SDL_SetRelativeMouseMode(SDL_TRUE);
                game_new();
            }
            if (action_pause()) {
                g_state=GS_MENU;
                menu_open_main();
            }
            break;
        }
        case GS_LEVEL_COMPLETE: {
            if (key_pressed(SDL_SCANCODE_RETURN)||key_pressed(SDL_SCANCODE_KP_ENTER)) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                game_next_level();
            }
            break;
        }
        default: break;
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (g_state==GS_PLAYING||g_state==GS_DEAD||g_state==GS_LEVEL_COMPLETE) {
            renderer_draw_frame(renderer, g_player);
            hud_draw(renderer, g_player, g_fps, g_frametime, g_state);
        }

        if (g_state==GS_MENU||g_state==GS_PAUSED) {
            if (g_state==GS_PAUSED) {
                renderer_draw_frame(renderer, g_player);
                SDL_SetRenderDrawColor(renderer,0,0,0,120);
                SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_BLEND);
                SDL_Rect full={0,0,SCREEN_W,SCREEN_H};
                SDL_RenderFillRect(renderer,&full);
            }
            menu_render(renderer, delta);
        }

        SDL_RenderPresent(renderer);
    }

    audio_shutdown();
    renderer_destroy();
    hud_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
