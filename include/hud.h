#pragma once
#include <SDL.h>
#include <SDL_ttf.h>
#include "common.h"
#include "player.h"
#include "world.h"
#include "entity.h"

static TTF_Font* g_font_large  = nullptr;
static TTF_Font* g_font_medium = nullptr;
static TTF_Font* g_font_small  = nullptr;
static bool      g_ttf_ok      = false;
static bool      g_show_debug  = false;
static bool      g_show_map    = false;

static const Color HUD_BG       = {15,  10,  20,  200};
static const Color HUD_ACCENT   = {200, 140,  20,  255};
static const Color HUD_RED      = {200,  30,  20,  255};
static const Color HUD_GREEN    = { 40, 180,  60,  255};
static const Color HUD_BLUE     = { 40, 100, 200,  255};
static const Color HUD_WHITE    = {220, 220, 220,  255};
static const Color HUD_GRAY     = {100, 100, 110,  255};
static const Color HUD_ORANGE   = {200,  80,  20,  255};
static const Color HUD_DARK     = { 10,   8,  15,  255};

inline bool hud_init() {
    if (TTF_Init() < 0) { g_ttf_ok=false; return false; }
    g_ttf_ok = true;
    return true;
}

inline void hud_shutdown() {
    if (!g_ttf_ok) return;
    if (g_font_large)  { TTF_CloseFont(g_font_large);  g_font_large=nullptr;  }
    if (g_font_medium) { TTF_CloseFont(g_font_medium); g_font_medium=nullptr; }
    if (g_font_small)  { TTF_CloseFont(g_font_small);  g_font_small=nullptr;  }
    TTF_Quit();
}

static void hud_draw_rect(SDL_Renderer* r, int x, int y, int w, int h, const Color& c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_Rect rc={x,y,w,h};
    SDL_RenderFillRect(r, &rc);
}

static void hud_draw_rect_outline(SDL_Renderer* r, int x, int y, int w, int h, const Color& c) {
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_Rect rc={x,y,w,h};
    SDL_RenderDrawRect(r, &rc);
}

static void hud_draw_text(SDL_Renderer* r, TTF_Font* font, const char* text,
                          int x, int y, const Color& c) {
    if (!font || !text) return;
    SDL_Color sc={c.r,c.g,c.b,c.a};
    SDL_Surface* surf = TTF_RenderText_Blended(font, text, sc);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
    if (tex) {
        SDL_Rect dst={x,y,surf->w,surf->h};
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    SDL_FreeSurface(surf);
}

static void hud_draw_bar(SDL_Renderer* r, int x, int y, int w, int h,
                         int val, int maxVal, const Color& fg, const Color& bg) {
    hud_draw_rect(r, x, y, w, h, bg);
    if (maxVal > 0) {
        int fw = (int)((double)val/maxVal * w);
        fw = std::max(0, std::min(w, fw));
        hud_draw_rect(r, x, y, fw, h, fg);
    }
    hud_draw_rect_outline(r, x, y, w, h, HUD_GRAY);
}

static void hud_draw_number_big(SDL_Renderer* r, int x, int y, int val, const Color& c) {
    char buf[32];
    SDL_snprintf(buf, sizeof(buf), "%d", val);
    hud_draw_text(r, g_font_large, buf, x, y, c);
}

static void render_hud_bottom(SDL_Renderer* r, const Player& p) {
    int bh = 90;
    int by = SCREEN_H - bh;
    hud_draw_rect(r, 0, by, SCREEN_W, bh, HUD_BG);

    SDL_SetRenderDrawColor(r, HUD_ACCENT.r, HUD_ACCENT.g, HUD_ACCENT.b, 120);
    SDL_RenderDrawLine(r, 0, by, SCREEN_W, by);

    hud_draw_rect(r, 10, by+5, 160, 80, {20,10,15,180});
    hud_draw_rect_outline(r, 10, by+5, 160, 80, HUD_RED);
    hud_draw_text(r, g_font_small, "HEALTH", 20, by+8, HUD_RED);
    Color hpCol = p.health>50 ? HUD_GREEN : p.health>25 ? HUD_ACCENT : HUD_RED;
    hud_draw_number_big(r, 20, by+22, p.health, hpCol);
    hud_draw_bar(r, 15, by+65, 150, 12, p.health, p.maxHealth, hpCol,{30,10,10,200});

    hud_draw_rect(r, 180, by+5, 160, 80, {10,10,25,180});
    hud_draw_rect_outline(r, 180, by+5, 160, 80, HUD_BLUE);
    hud_draw_text(r, g_font_small, "ARMOR", 190, by+8, HUD_BLUE);
    Color arCol = p.armor>50 ? HUD_BLUE : HUD_GRAY;
    hud_draw_number_big(r, 190, by+22, p.armor, arCol);
    hud_draw_bar(r, 185, by+65, 150, 12, p.armor, p.maxArmor, arCol,{10,10,30,200});

    const Weapon& w = p.weapons[p.currentWeapon];
    hud_draw_rect(r, 350, by+5, 200, 80, {20,15,10,180});
    hud_draw_rect_outline(r, 350, by+5, 200, 80, HUD_ORANGE);
    hud_draw_text(r, g_font_small, weapon_names[p.currentWeapon], 360, by+8, HUD_ACCENT);
    bool isMelee = (p.currentWeapon==0||p.currentWeapon==13);
    if (isMelee) {
        hud_draw_text(r, g_font_large, "---", 360, by+22, HUD_GRAY);
    } else {
        hud_draw_number_big(r, 360, by+22, w.ammo, w.ammo>0?HUD_WHITE:HUD_RED);
        char maxbuf[32];
        SDL_snprintf(maxbuf,sizeof(maxbuf),"/ %d", w.maxAmmo);
        hud_draw_text(r, g_font_medium, maxbuf, 420, by+38, HUD_GRAY);
    }
    if (!isMelee) {
        hud_draw_bar(r, 355, by+65, 190, 12,
            w.ammo, w.maxAmmo,
            w.ammo>0?HUD_ORANGE:HUD_RED, {30,15,5,200});
    }

    hud_draw_rect(r, SCREEN_W-320, by+5, 310, 80, {10,15,10,180});
    hud_draw_rect_outline(r, SCREEN_W-320, by+5, 310, 80, HUD_GREEN);
    hud_draw_text(r, g_font_small, "SCORE", SCREEN_W-310, by+8, HUD_GREEN);
    hud_draw_number_big(r, SCREEN_W-310, by+22, p.score, HUD_ACCENT);
    char killbuf[64];
    SDL_snprintf(killbuf,sizeof(killbuf),"KILLS: %d    LVL: %d", p.kills, p.level);
    hud_draw_text(r, g_font_small, killbuf, SCREEN_W-310, by+58, HUD_GRAY);
}

static void render_weapon_hud(SDL_Renderer* r, const Player& p) {
    int wx=SCREEN_W/2-160, wy=SCREEN_H/2-120;
    int ww=320, wh=240;
    hud_draw_rect(r, wx, wy, ww, wh, {5,3,10,220});
    hud_draw_rect_outline(r, wx, wy, ww, wh, HUD_ACCENT);
    hud_draw_text(r, g_font_medium, "[ WEAPONS ]", wx+90, wy+6, HUD_ACCENT);

    for (int i=0;i<MAX_WEAPONS;i++) {
        const Weapon& w = p.weapons[i];
        int row=i%8, col=i/8;
        int ix=wx+8+col*160, iy=wy+30+row*25;
        bool cur=(i==p.currentWeapon);
        bool owned=w.owned;
        if (cur) hud_draw_rect(r, ix-2, iy-2, 154, 22, {60,40,10,180});
        Color nc = owned ? (cur?HUD_ACCENT:HUD_WHITE) : HUD_GRAY;
        char wbuf[64];
        if (owned && !isinf(w.ammo) && w.maxAmmo>0)
            SDL_snprintf(wbuf,sizeof(wbuf),"%d %s [%d]",i+1,weapon_names[i],w.ammo);
        else if (owned)
            SDL_snprintf(wbuf,sizeof(wbuf),"%d %s",i+1,weapon_names[i]);
        else
            SDL_snprintf(wbuf,sizeof(wbuf),"%d ---",i+1);
        hud_draw_text(r, g_font_small, wbuf, ix+2, iy, nc);
    }
    hud_draw_text(r, g_font_small, "[E] SWAP FROM FLOOR   [WHEEL/1-9] SELECT",
                  wx+8, wy+wh-20, HUD_GRAY);
}

static void render_minimap(SDL_Renderer* r, const Player& p) {
    const int CELL = 6;
    const int MAP_DRAW = 32;
    int mx = SCREEN_W/2 - MAP_DRAW*CELL/2;
    int my = 20;
    int mw = MAP_DRAW*CELL;
    int mh = MAP_DRAW*CELL;

    hud_draw_rect(r, mx-4, my-4, mw+8, mh+8, {5,3,10,220});
    hud_draw_rect_outline(r, mx-4, my-4, mw+8, mh+8, HUD_ACCENT);

    int camX=(int)p.pos.x - MAP_DRAW/2;
    int camY=(int)p.pos.y - MAP_DRAW/2;

    for (int y=0;y<MAP_DRAW;y++)
    for (int x=0;x<MAP_DRAW;x++) {
        int wx2=camX+x, wy2=camY+y;
        if (!world_in_bounds(wx2,wy2)) continue;
        Tile& t=g_world.map[wy2][wx2];
        Color tc={0,0,0,0};
        switch(t.type){
        case TILE_WALL:  tc={60,60,80,200};  break;
        case TILE_EMPTY: tc={30,30,40,200};  break;
        case TILE_DOOR:  tc={100,80,20,200}; break;
        case TILE_EXIT:  tc={20,180,20,200}; break;
        }
        if(tc.a==0) continue;
        SDL_Rect rc2={mx+x*CELL, my+y*CELL, CELL-1, CELL-1};
        SDL_SetRenderDrawColor(r,tc.r,tc.g,tc.b,tc.a);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(r,&rc2);
    }

    for (int i=0;i<g_entityCount;i++){
        Entity& e=g_entities[i];
        if(!e.active||e.state==ES_DEAD) continue;
        int ex2=(int)(e.pos.x)-camX;
        int ey2=(int)(e.pos.y)-camY;
        if(ex2<0||ex2>=MAP_DRAW||ey2<0||ey2>=MAP_DRAW) continue;
        Color ec = e.type==ENT_ENEMY ? HUD_RED : HUD_GREEN;
        SDL_Rect er={mx+ex2*CELL+1,my+ey2*CELL+1,CELL-2,CELL-2};
        SDL_SetRenderDrawColor(r,ec.r,ec.g,ec.b,220);
        SDL_RenderFillRect(r,&er);
    }

    int px2=(int)p.pos.x-camX;
    int py2=(int)p.pos.y-camY;
    SDL_SetRenderDrawColor(r,255,255,80,255);
    SDL_Rect pr={mx+px2*CELL-1,my+py2*CELL-1,CELL+1,CELL+1};
    SDL_RenderFillRect(r,&pr);

    double dex=p.dir.x*CELL*1.5, dey=p.dir.y*CELL*1.5;
    SDL_SetRenderDrawColor(r,255,200,0,255);
    SDL_RenderDrawLine(r,
        mx+px2*CELL+CELL/2, my+py2*CELL+CELL/2,
        mx+px2*CELL+CELL/2+(int)dex, my+py2*CELL+CELL/2+(int)dey);

    char lbuf[32];
    SDL_snprintf(lbuf,sizeof(lbuf),"LEVEL %d", p.level);
    hud_draw_text(r, g_font_small, lbuf, mx, my+mh+4, HUD_ACCENT);
}

static void render_debug_overlay(SDL_Renderer* r, const Player& p, const DebugInfo& di) {
    int px=8, py=8, lh=18;
    hud_draw_rect(r, 0, 0, 380, 330, {5,3,10,210});
    hud_draw_rect_outline(r, 0, 0, 380, 330, HUD_ACCENT);

    char buf[128];
    auto line=[&](const char* s, const Color& c=HUD_WHITE){
        hud_draw_text(r,g_font_small,s,px,py,c); py+=lh;
    };

    SDL_snprintf(buf,sizeof(buf),"Null Zone %s  [F3 to hide]", NZ_VERSION);
    line(buf, HUD_ACCENT);
    SDL_snprintf(buf,sizeof(buf),"FPS: %.1f  Frame: %.2f ms", di.fps, di.frametime*1000.0);
    line(buf, di.fps>=55?HUD_GREEN:di.fps>=30?HUD_ACCENT:HUD_RED);
    line("", HUD_GRAY);
    SDL_snprintf(buf,sizeof(buf),"XYZ: %.3f / %.3f", di.playerX, di.playerY);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"Angle: %.2f deg", di.playerAngle*180.0/M_PI);
    line(buf);
    line("", HUD_GRAY);
    SDL_snprintf(buf,sizeof(buf),"Level: %d   Seed: %u", di.level, di.seed);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"Entities total:   %d", di.totalEntities);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"Entities aggro:   %d", di.aggroEntities);
    line(buf, HUD_ORANGE);
    SDL_snprintf(buf,sizeof(buf),"Entities visible: %d", di.visibleEntities);
    line(buf, HUD_GREEN);
    SDL_snprintf(buf,sizeof(buf),"Entities frozen:  %d", di.frozenEntities);
    line(buf, HUD_GRAY);
    SDL_snprintf(buf,sizeof(buf),"Active rooms:     %d", di.activeRooms);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"Memory:           %zu KB", di.memUsageKB);
    line(buf);
    line("", HUD_GRAY);
    SDL_snprintf(buf,sizeof(buf),"Weapon: %s", weapon_names[di.currentWeapon]);
    line(buf, HUD_ACCENT);
    SDL_snprintf(buf,sizeof(buf),"Ammo:   %d", di.currentAmmo);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"HP: %d  Armor: %d", p.health, p.armor);
    line(buf, HUD_GREEN);
}

static void render_level_complete(SDL_Renderer* r, const Player& p) {
    hud_draw_rect(r, SCREEN_W/2-220, SCREEN_H/2-80, 440, 160, {5,3,10,230});
    hud_draw_rect_outline(r, SCREEN_W/2-220, SCREEN_H/2-80, 440, 160, HUD_ACCENT);
    hud_draw_text(r, g_font_large, "LEVEL COMPLETE!", SCREEN_W/2-180, SCREEN_H/2-65, HUD_ACCENT);
    char buf[64];
    SDL_snprintf(buf,sizeof(buf),"KILLS: %d    SCORE: %d", p.kills, p.score);
    hud_draw_text(r, g_font_medium, buf, SCREEN_W/2-160, SCREEN_H/2-10, HUD_WHITE);
    hud_draw_text(r, g_font_small, "Press ENTER to continue", SCREEN_W/2-120, SCREEN_H/2+40, HUD_GRAY);
}

static void render_dead_screen(SDL_Renderer* r, const Player& p) {
    hud_draw_rect(r, 0, 0, SCREEN_W, SCREEN_H, {80,0,0,120});
    hud_draw_rect(r, SCREEN_W/2-200, SCREEN_H/2-80, 400, 160, {20,3,3,240});
    hud_draw_rect_outline(r, SCREEN_W/2-200, SCREEN_H/2-80, 400, 160, HUD_RED);
    hud_draw_text(r, g_font_large, "YOU DIED", SCREEN_W/2-120, SCREEN_H/2-65, HUD_RED);
    char buf[64];
    SDL_snprintf(buf,sizeof(buf),"Score: %d   Kills: %d   Level: %d",
                 p.score, p.kills, p.level);
    hud_draw_text(r, g_font_small, buf, SCREEN_W/2-160, SCREEN_H/2-5, HUD_GRAY);
    hud_draw_text(r, g_font_small, "Press R to restart   ESC for menu",
                  SCREEN_W/2-140, SCREEN_H/2+40, HUD_WHITE);
}

inline DebugInfo hud_build_debug(const Player& p, double fps, double ft) {
    DebugInfo di;
    di.fps            = fps;
    di.frametime      = ft;
    di.playerX        = p.pos.x;
    di.playerY        = p.pos.y;
    di.playerAngle    = p.angle;
    di.level          = p.level;
    di.seed           = g_world.seed;
    di.currentWeapon  = p.currentWeapon;
    di.currentAmmo    = p.weapons[p.currentWeapon].ammo;
    di.totalEntities  = g_entityCount;
    di.aggroEntities  = entities_count_aggro();
    di.frozenEntities = entities_count_frozen();
    di.visibleEntities= 0;
    for(int i=0;i<g_entityCount;i++)
        if(g_entities[i].active&&g_entities[i].inFOV) di.visibleEntities++;
    di.activeRooms=0;
    for(int i=0;i<g_world.roomCount;i++)
        if(g_world.rooms[i].state==ROOM_VISIBLE) di.activeRooms++;
    di.memUsageKB = (sizeof(g_world)+sizeof(g_entities)+sizeof(g_textures))/1024;
    return di;
}

inline void hud_draw(SDL_Renderer* r, const Player& p,
                     double fps, double ft, GameState state) {
    if (state==GS_PLAYING || state==GS_PAUSED) {
        render_hud_bottom(r, p);
        if (g_show_map)   render_minimap(r, p);
        if (action_wpnhud()) render_weapon_hud(r, p);
        if (g_show_debug) {
            DebugInfo di = hud_build_debug(p, fps, ft);
            render_debug_overlay(r, p, di);
        }
    }
    if (state==GS_LEVEL_COMPLETE) render_level_complete(r, p);
    if (state==GS_DEAD)           render_dead_screen(r, p);
}

inline void hud_toggle_debug() { g_show_debug = !g_show_debug; }
inline void hud_toggle_map()   { g_show_map   = !g_show_map;   }
