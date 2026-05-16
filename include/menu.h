#pragma once
#include <SDL.h>
#include "common.h"
#include "hud.h"
#include "savegame.h"
#include "input.h"

enum MenuID {
    MENU_MAIN,
    MENU_PAUSE,
    MENU_SAVE,
    MENU_LOAD,
    MENU_SETTINGS,
    MENU_CONFIRM_DELETE,
    MENU_CONFIRM_QUIT,
    MENU_ERROR
};

struct MenuState {
    MenuID  current;
    int     selectedItem;
    int     selectedSlot;
    bool    deleteConfirm;
    char    errorMsg[256];
    double  animTimer;
    GameState returnState;
    SaveLoadResult lastLoadResult;
};

static MenuState g_menu = {};

static const char* main_items[]  = {
    "NEW GAME", "CONTINUE", "LOAD GAME", "SETTINGS", "QUIT"
};
static const int MAIN_COUNT = 5;

static const char* pause_items[] = {
    "RESUME", "SAVE GAME", "LOAD GAME", "MAIN MENU", "QUIT TO DESKTOP"
};
static const int PAUSE_COUNT = 5;

static const char* settings_items[] = {
    "MOUSE SENSITIVITY: MEDIUM",
    "SOUND VOLUME: 100%",
    "MUSIC VOLUME: 100%",
    "RESOLUTION: 1280x720",
    "BACK"
};
static const int SETTINGS_COUNT = 5;

static int  g_mouse_sens    = 2;
static int  g_sound_vol     = 100;
static int  g_music_vol     = 100;

inline void menu_init() {
    memset(&g_menu, 0, sizeof(g_menu));
    g_menu.current     = MENU_MAIN;
    g_menu.selectedItem= 0;
    g_menu.selectedSlot= 1;
    g_menu.animTimer   = 0.0;
    g_menu.returnState = GS_MENU;
}

static void draw_logo(SDL_Renderer* r, double anim) {
    int cx = SCREEN_W/2;
    int ty = 60;
    double pulse = 1.0 + 0.04*sin(anim*2.0);

    hud_draw_rect(r, cx-220, ty-10, 440, 90, {5,3,10,200});

    Color c1 = {200, (uint8_t)(140+sin(anim)*20), 20, 255};
    Color c2 = {(uint8_t)(180+sin(anim*1.3)*20), 50, 20, 255};

    char title[] = "NULL ZONE";
    int tw = (int)(strlen(title)*28*pulse);
    hud_draw_text(r, g_font_large, title, cx - tw/2, ty, c1);

    char sub[] = "v" NZ_VERSION "  by " NZ_AUTHOR;
    hud_draw_text(r, g_font_small, sub, cx-120, ty+58, c2);
}

static void draw_menu_items(SDL_Renderer* r, const char** items, int count,
                            int selected, int startY) {
    for (int i=0;i<count;i++) {
        int iy = startY + i*50;
        bool sel = (i==selected);

        if (sel) {
            hud_draw_rect(r, SCREEN_W/2-180, iy-6, 360, 42, {40,25,5,200});
            hud_draw_rect_outline(r, SCREEN_W/2-180, iy-6, 360, 42, HUD_ACCENT);
        }

        Color tc = sel ? HUD_ACCENT : HUD_WHITE;
        if (sel) {
            hud_draw_text(r, g_font_medium, ">", SCREEN_W/2-165, iy, HUD_ORANGE);
            hud_draw_text(r, g_font_medium, "<", SCREEN_W/2+148, iy, HUD_ORANGE);
        }
        int tw = (int)strlen(items[i]) * 10;
        hud_draw_text(r, g_font_medium, items[i], SCREEN_W/2-tw/2, iy, tc);
    }
}

static void draw_save_load_menu(SDL_Renderer* r, bool isSave, int selectedSlot, double anim) {
    int bx = SCREEN_W/2-300, by=120, bw=600, bh=460;
    hud_draw_rect(r, bx, by, bw, bh, {5,3,10,230});
    hud_draw_rect_outline(r, bx, by, bw, bh, HUD_ACCENT);

    const char* title = isSave ? "[ SAVE GAME ]" : "[ LOAD GAME ]";
    hud_draw_text(r, g_font_large, title, bx+180, by+12, HUD_ACCENT);

    hud_draw_text(r, g_font_small, "AUTO", bx+12, by+52, HUD_GRAY);
    char autobuf[128];
    savegame_get_slot_info(0, autobuf, sizeof(autobuf));
    bool autoSel = (selectedSlot==0);
    if (autoSel) hud_draw_rect(r, bx+8, by+48, bw-16, 30, {40,25,5,180});
    hud_draw_text(r, g_font_small, autobuf, bx+70, by+52,
                  autoSel?HUD_ACCENT:HUD_GRAY);

    SDL_SetRenderDrawColor(r, HUD_GRAY.r, HUD_GRAY.g, HUD_GRAY.b, 80);
    SDL_RenderDrawLine(r, bx+8, by+84, bx+bw-8, by+84);

    for (int i=1;i<=MAX_SAVE_SLOTS;i++) {
        int sy = by+90+(i-1)*44;
        bool sel=(i==selectedSlot);

        if (sel) {
            hud_draw_rect(r, bx+8, sy-2, bw-16, 40, {40,25,5,200});
            hud_draw_rect_outline(r, bx+8, sy-2, bw-16, 40, HUD_ACCENT);
        }

        char slotlabel[16];
        SDL_snprintf(slotlabel,sizeof(slotlabel),"SLOT %d",i);
        hud_draw_text(r, g_font_small, slotlabel, bx+14, sy+2,
                      sel?HUD_ACCENT:HUD_GRAY);

        char info[128];
        savegame_get_slot_info((uint8_t)i, info, sizeof(info));
        hud_draw_text(r, g_font_small, info, bx+80, sy+2,
                      sel?(savegame_slot_exists((uint8_t)i)?HUD_WHITE:HUD_GRAY):HUD_GRAY);

        if (sel && savegame_slot_exists((uint8_t)i)) {
            hud_draw_text(r, g_font_small, "[DEL] Delete",
                          bx+bw-120, sy+2, HUD_RED);
        }
    }

    const char* hint = isSave
        ? "ENTER: Save   DEL: Delete slot   ESC: Back"
        : "ENTER: Load   DEL: Delete slot   ESC: Back";
    hud_draw_text(r, g_font_small, hint, bx+40, by+bh-24, HUD_GRAY);

    double pulse = 0.6+0.4*sin(anim*3.0);
    Color pc = {(uint8_t)(200*pulse),(uint8_t)(140*pulse),20,255};
    hud_draw_text(r, g_font_small, "UP/DOWN: Select slot", bx+40, by+bh-42, pc);
}

static void draw_confirm_delete(SDL_Renderer* r, int slot) {
    hud_draw_rect(r, SCREEN_W/2-200, SCREEN_H/2-70, 400, 140, {20,5,5,240});
    hud_draw_rect_outline(r, SCREEN_W/2-200, SCREEN_H/2-70, 400, 140, HUD_RED);
    char buf[64];
    SDL_snprintf(buf,sizeof(buf),"Delete slot %d?", slot);
    hud_draw_text(r, g_font_medium, buf, SCREEN_W/2-80, SCREEN_H/2-55, HUD_RED);
    hud_draw_text(r, g_font_small,
                  "ENTER: Confirm   ESC: Cancel",
                  SCREEN_W/2-120, SCREEN_H/2+10, HUD_WHITE);
}

static void draw_error(SDL_Renderer* r, const char* msg) {
    hud_draw_rect(r, SCREEN_W/2-240, SCREEN_H/2-60, 480, 120, {20,5,5,240});
    hud_draw_rect_outline(r, SCREEN_W/2-240, SCREEN_H/2-60, 480, 120, HUD_RED);
    hud_draw_text(r, g_font_medium, "ERROR", SCREEN_W/2-40, SCREEN_H/2-48, HUD_RED);
    hud_draw_text(r, g_font_small,  msg,     SCREEN_W/2-200, SCREEN_H/2-10, HUD_WHITE);
    hud_draw_text(r, g_font_small,  "Press any key to continue",
                  SCREEN_W/2-110, SCREEN_H/2+30, HUD_GRAY);
}

static void draw_settings(SDL_Renderer* r, int selected) {
    static const char* sens_names[] = {"LOW","MEDIUM","HIGH","VERY HIGH"};
    char items[SETTINGS_COUNT][64];
    SDL_snprintf(items[0],64,"MOUSE SENSITIVITY: %s", sens_names[std::min(g_mouse_sens,3)]);
    SDL_snprintf(items[1],64,"SOUND VOLUME: %d%%", g_sound_vol);
    SDL_snprintf(items[2],64,"MUSIC VOLUME: %d%%", g_music_vol);
    SDL_snprintf(items[3],64,"RESOLUTION: 1280x720");
    SDL_snprintf(items[4],64,"BACK");
    const char* ptrs[SETTINGS_COUNT];
    for(int i=0;i<SETTINGS_COUNT;i++) ptrs[i]=items[i];
    draw_menu_items(r, ptrs, SETTINGS_COUNT, selected, 220);
}

inline void menu_render(SDL_Renderer* r, double dt) {
    g_menu.animTimer += dt;
    double anim = g_menu.animTimer;

    hud_draw_rect(r, 0, 0, SCREEN_W, SCREEN_H, HUD_DARK);

    for (int i=0;i<80;i++) {
        uint32_t rx = (uint32_t)(i*73+13)^(uint32_t)(anim*3+i);
        rx ^= rx<<13; rx^=rx>>17; rx^=rx<<5;
        int sx=(int)(rx%SCREEN_W), sy=(int)((rx>>12)%SCREEN_H);
        uint8_t bri=(uint8_t)((rx>>20)%80+10);
        if((int)(anim*2+i)%3==0) bri=(uint8_t)(bri*((sin(anim*2+i)*0.5+0.5)*0.8+0.2));
        SDL_SetRenderDrawColor(r,bri,bri,(uint8_t)(bri+20),255);
        SDL_RenderDrawPoint(r,sx,sy);
    }

    switch(g_menu.current) {
    case MENU_MAIN:
        draw_logo(r, anim);
        hud_draw_rect(r, SCREEN_W/2-200, 155, 400, 2, HUD_ACCENT);
        draw_menu_items(r, main_items, MAIN_COUNT, g_menu.selectedItem, 200);
        hud_draw_text(r, g_font_small,
            NZ_AUTHOR "  |  " NZ_EMAIL "  |  " NZ_YEAR,
            SCREEN_W/2-220, SCREEN_H-28, HUD_GRAY);
        break;

    case MENU_PAUSE:
        hud_draw_rect(r, SCREEN_W/2-220, 60, 440, 60, {5,3,10,220});
        hud_draw_rect_outline(r, SCREEN_W/2-220, 60, 440, 60, HUD_ACCENT);
        hud_draw_text(r, g_font_large, "PAUSED", SCREEN_W/2-80, 72, HUD_ACCENT);
        draw_menu_items(r, pause_items, PAUSE_COUNT, g_menu.selectedItem, 180);
        break;

    case MENU_SAVE:
        draw_save_load_menu(r, true, g_menu.selectedSlot, anim);
        if (g_menu.deleteConfirm) draw_confirm_delete(r, g_menu.selectedSlot);
        break;

    case MENU_LOAD:
        draw_save_load_menu(r, false, g_menu.selectedSlot, anim);
        if (g_menu.deleteConfirm) draw_confirm_delete(r, g_menu.selectedSlot);
        break;

    case MENU_SETTINGS:
        draw_logo(r, anim);
        hud_draw_text(r, g_font_large, "SETTINGS", SCREEN_W/2-80, 160, HUD_ACCENT);
        draw_settings(r, g_menu.selectedItem);
        break;

    case MENU_ERROR:
        draw_error(r, g_menu.errorMsg);
        break;

    default: break;
    }
}

enum MenuAction {
    MACT_NONE,
    MACT_NEW_GAME,
    MACT_CONTINUE,
    MACT_RESUME,
    MACT_LOAD_OK,
    MACT_GOTO_MAIN,
    MACT_QUIT
};

inline MenuAction menu_update(Player& p, GameState& gs) {
    bool up    = key_pressed(SDL_SCANCODE_UP)   || key_pressed(SDL_SCANCODE_W);
    bool down  = key_pressed(SDL_SCANCODE_DOWN) || key_pressed(SDL_SCANCODE_S);
    bool enter = key_pressed(SDL_SCANCODE_RETURN)||key_pressed(SDL_SCANCODE_KP_ENTER);
    bool esc   = action_pause();
    bool del   = key_pressed(SDL_SCANCODE_DELETE);
    bool left  = key_pressed(SDL_SCANCODE_LEFT) || key_pressed(SDL_SCANCODE_A);
    bool right = key_pressed(SDL_SCANCODE_RIGHT)|| key_pressed(SDL_SCANCODE_D);

    if (g_menu.current == MENU_ERROR) {
        if (enter||esc||del||up||down)
            g_menu.current = (g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        return MACT_NONE;
    }

    if (g_menu.deleteConfirm) {
        if (enter) {
            savegame_delete((uint8_t)g_menu.selectedSlot);
            g_menu.deleteConfirm=false;
        } else if (esc) {
            g_menu.deleteConfirm=false;
        }
        return MACT_NONE;
    }

    switch(g_menu.current) {
    case MENU_MAIN: {
        if (up)   g_menu.selectedItem=(g_menu.selectedItem-1+MAIN_COUNT)%MAIN_COUNT;
        if (down) g_menu.selectedItem=(g_menu.selectedItem+1)%MAIN_COUNT;
        if (enter) {
            switch(g_menu.selectedItem){
            case 0: return MACT_NEW_GAME;
            case 1:
                if (savegame_slot_exists(0)||savegame_slot_exists(1)) {
                    SaveLoadResult res=savegame_read(0,p);
                    if(res==SAVE_OK){gs=GS_PLAYING;return MACT_CONTINUE;}
                    res=savegame_read(1,p);
                    if(res==SAVE_OK){gs=GS_PLAYING;return MACT_CONTINUE;}
                    SDL_strlcpy(g_menu.errorMsg,"No save found",sizeof(g_menu.errorMsg));
                    g_menu.returnState=GS_MENU;
                    g_menu.current=MENU_ERROR;
                } else {
                    SDL_strlcpy(g_menu.errorMsg,"No save file found. Start a new game.",sizeof(g_menu.errorMsg));
                    g_menu.returnState=GS_MENU;
                    g_menu.current=MENU_ERROR;
                }
                break;
            case 2: g_menu.current=MENU_LOAD; g_menu.selectedSlot=1; break;
            case 3: g_menu.current=MENU_SETTINGS; g_menu.selectedItem=0; break;
            case 4: return MACT_QUIT;
            }
        }
        break;
    }
    case MENU_PAUSE: {
        if (up)   g_menu.selectedItem=(g_menu.selectedItem-1+PAUSE_COUNT)%PAUSE_COUNT;
        if (down) g_menu.selectedItem=(g_menu.selectedItem+1)%PAUSE_COUNT;
        if (esc) return MACT_RESUME;
        if (enter) {
            switch(g_menu.selectedItem){
            case 0: return MACT_RESUME;
            case 1: g_menu.current=MENU_SAVE; g_menu.selectedSlot=1; break;
            case 2: g_menu.current=MENU_LOAD; g_menu.selectedSlot=1; break;
            case 3: gs=GS_MENU; g_menu.current=MENU_MAIN; g_menu.selectedItem=0; return MACT_GOTO_MAIN;
            case 4: return MACT_QUIT;
            }
        }
        break;
    }
    case MENU_SAVE: {
        int maxSlot=MAX_SAVE_SLOTS;
        if (up)   g_menu.selectedSlot=std::max(0,g_menu.selectedSlot-1);
        if (down) g_menu.selectedSlot=std::min(maxSlot,g_menu.selectedSlot+1);
        if (del && g_menu.selectedSlot>0 && savegame_slot_exists((uint8_t)g_menu.selectedSlot))
            g_menu.deleteConfirm=true;
        if (enter && g_menu.selectedSlot>0) {
            savegame_write(p,(uint8_t)g_menu.selectedSlot);
            g_menu.current=MENU_PAUSE;
        }
        if (esc) g_menu.current=MENU_PAUSE;
        break;
    }
    case MENU_LOAD: {
        int maxSlot=MAX_SAVE_SLOTS;
        if (up)   g_menu.selectedSlot=std::max(0,g_menu.selectedSlot-1);
        if (down) g_menu.selectedSlot=std::min(maxSlot,g_menu.selectedSlot+1);
        if (del && g_menu.selectedSlot>0 && savegame_slot_exists((uint8_t)g_menu.selectedSlot))
            g_menu.deleteConfirm=true;
        if (enter) {
            SaveLoadResult res=savegame_read((uint8_t)g_menu.selectedSlot,p);
            if(res==SAVE_OK){
                gs=GS_PLAYING;
                return MACT_LOAD_OK;
            } else {
                SDL_snprintf(g_menu.errorMsg,sizeof(g_menu.errorMsg),
                    "Failed to load: %s",savegame_error_str(res));
                g_menu.returnState=gs;
                g_menu.current=MENU_ERROR;
            }
        }
        if (esc) g_menu.current=(g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        break;
    }
    case MENU_SETTINGS: {
        if (up)   g_menu.selectedItem=(g_menu.selectedItem-1+SETTINGS_COUNT)%SETTINGS_COUNT;
        if (down) g_menu.selectedItem=(g_menu.selectedItem+1)%SETTINGS_COUNT;
        if (enter||esc) {
            if(g_menu.selectedItem==SETTINGS_COUNT-1||esc)
                g_menu.current=(g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        }
        if (left||right) {
            int d=(right?1:-1);
            switch(g_menu.selectedItem){
            case 0: g_mouse_sens=std::max(0,std::min(3,g_mouse_sens+d)); break;
            case 1: g_sound_vol=std::max(0,std::min(100,g_sound_vol+d*10)); break;
            case 2: g_music_vol=std::max(0,std::min(100,g_music_vol+d*10)); break;
            }
        }
        break;
    }
    default: break;
    }
    return MACT_NONE;
}

inline void menu_open_pause(GameState returnSt) {
    g_menu.current     = MENU_PAUSE;
    g_menu.selectedItem= 0;
    g_menu.returnState = returnSt;
    g_menu.deleteConfirm=false;
}

inline void menu_open_main() {
    g_menu.current     = MENU_MAIN;
    g_menu.selectedItem= 0;
    g_menu.deleteConfirm=false;
}
