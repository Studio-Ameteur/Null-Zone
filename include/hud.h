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

static const Color HUD_BG     = {10,  8,  18, 210};
static const Color HUD_ACCENT = {200,140,  20, 255};
static const Color HUD_RED    = {200, 30,  20, 255};
static const Color HUD_GREEN  = { 40,180,  60, 255};
static const Color HUD_BLUE   = { 40,100, 200, 255};
static const Color HUD_WHITE  = {220,220, 220, 255};
static const Color HUD_GRAY   = {100,100, 110, 255};
static const Color HUD_ORANGE = {200, 80,  20, 255};
static const Color HUD_DARK   = { 8,  6,  14, 255};

static int g_sw=SCREEN_W, g_sh=SCREEN_H;

inline void hud_set_size(int w, int h) { g_sw=w; g_sh=h; }

inline bool hud_init() {
    if(TTF_Init()<0){g_ttf_ok=false;return false;}
    g_ttf_ok=true; return true;
}

inline void hud_shutdown() {
    if(!g_ttf_ok) return;
    if(g_font_large) {TTF_CloseFont(g_font_large); g_font_large=nullptr;}
    if(g_font_medium){TTF_CloseFont(g_font_medium);g_font_medium=nullptr;}
    if(g_font_small) {TTF_CloseFont(g_font_small); g_font_small=nullptr;}
    TTF_Quit();
}

static void hud_rect(SDL_Renderer* r, int x,int y,int w,int h,const Color& c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    SDL_Rect rc={x,y,w,h}; SDL_RenderFillRect(r,&rc);
}

static void hud_rect_out(SDL_Renderer* r, int x,int y,int w,int h,const Color& c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_Rect rc={x,y,w,h}; SDL_RenderDrawRect(r,&rc);
}

static void hud_line(SDL_Renderer* r, int x1,int y1,int x2,int y2,const Color& c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_RenderDrawLine(r,x1,y1,x2,y2);
}

static int hud_text(SDL_Renderer* r, TTF_Font* f, const char* txt, int x,int y,const Color& c) {
    if(!f||!txt) return 0;
    SDL_Color sc={c.r,c.g,c.b,c.a};
    SDL_Surface* surf=TTF_RenderText_Blended(f,txt,sc);
    if(!surf) return 0;
    SDL_Texture* tex=SDL_CreateTextureFromSurface(r,surf);
    int tw=surf->w;
    if(tex){SDL_Rect dst={x,y,surf->w,surf->h};SDL_RenderCopy(r,tex,nullptr,&dst);SDL_DestroyTexture(tex);}
    SDL_FreeSurface(surf);
    return tw;
}

static void hud_text_center(SDL_Renderer* r, TTF_Font* f, const char* txt, int cx,int y,const Color& c) {
    if(!f||!txt) return;
    int w,h; TTF_SizeText(f,txt,&w,&h);
    hud_text(r,f,txt,cx-w/2,y,c);
}

static void hud_bar(SDL_Renderer* r, int x,int y,int w,int h,int val,int maxv,const Color& fg,const Color& bg) {
    hud_rect(r,x,y,w,h,bg);
    if(maxv>0){int fw=std::max(0,std::min(w,(int)((double)val/maxv*w)));hud_rect(r,x,y,fw,h,fg);}
    hud_rect_out(r,x,y,w,h,HUD_GRAY);
}

static void hud_number(SDL_Renderer* r, TTF_Font* f, int val, int x, int y, const Color& c) {
    char buf[32]; SDL_snprintf(buf,sizeof(buf),"%d",val);
    hud_text(r,f,buf,x,y,c);
}

static void draw_hud_bottom(SDL_Renderer* r, const Player& p) {
    int bh=g_sh/8;
    int by=g_sh-bh;
    int pad=g_sw/100;

    hud_rect(r,0,by,g_sw,bh,HUD_BG);
    hud_line(r,0,by,g_sw,by,HUD_ACCENT);

    int sec_w=g_sw/5;

    hud_rect(r,pad,by+pad,sec_w-pad*2,bh-pad*2,{20,8,12,180});
    hud_rect_out(r,pad,by+pad,sec_w-pad*2,bh-pad*2,HUD_RED);
    hud_text(r,g_font_small,"HEALTH",pad*2,by+pad*2,HUD_RED);
    Color hpc=p.health>50?HUD_GREEN:p.health>25?HUD_ACCENT:HUD_RED;
    hud_number(r,g_font_large,p.health,pad*2,by+bh/4,hpc);
    hud_bar(r,pad*2,by+bh-bh/5,sec_w-pad*4,bh/8,p.health,p.maxHealth,hpc,{30,8,8,200});

    int x2=sec_w+pad;
    hud_rect(r,x2,by+pad,sec_w-pad*2,bh-pad*2,{8,10,22,180});
    hud_rect_out(r,x2,by+pad,sec_w-pad*2,bh-pad*2,HUD_BLUE);
    hud_text(r,g_font_small,"ARMOR",x2+pad,by+pad*2,HUD_BLUE);
    Color arc=p.armor>50?HUD_BLUE:HUD_GRAY;
    hud_number(r,g_font_large,p.armor,x2+pad,by+bh/4,arc);
    hud_bar(r,x2+pad,by+bh-bh/5,sec_w-pad*4,bh/8,p.armor,p.maxArmor,arc,{8,8,28,200});

    int x3=sec_w*2+pad;
    const Weapon& w=p.weapons[p.currentWeapon];
    hud_rect(r,x3,by+pad,sec_w-pad*2,bh-pad*2,{18,12,5,180});
    hud_rect_out(r,x3,by+pad,sec_w-pad*2,bh-pad*2,HUD_ORANGE);
    hud_text(r,g_font_small,weapon_names[p.currentWeapon],x3+pad,by+pad*2,HUD_ACCENT);
    bool melee=(p.currentWeapon==0||p.currentWeapon==13);
    if(melee) hud_text(r,g_font_large,"---",x3+pad,by+bh/4,HUD_GRAY);
    else {
        hud_number(r,g_font_large,w.ammo,x3+pad,by+bh/4,w.ammo>0?HUD_WHITE:HUD_RED);
        char mb[32]; SDL_snprintf(mb,sizeof(mb),"/%d",w.maxAmmo);
        hud_text(r,g_font_medium,mb,x3+pad*8,by+bh/3,HUD_GRAY);
        hud_bar(r,x3+pad,by+bh-bh/5,sec_w-pad*4,bh/8,w.ammo,w.maxAmmo,w.ammo>0?HUD_ORANGE:HUD_RED,{28,12,4,200});
    }

    int x4=g_sw-sec_w-pad;
    hud_rect(r,x4,by+pad,sec_w-pad*2,bh-pad*2,{8,16,8,180});
    hud_rect_out(r,x4,by+pad,sec_w-pad*2,bh-pad*2,HUD_GREEN);
    hud_text(r,g_font_small,"SCORE",x4+pad,by+pad*2,HUD_GREEN);
    hud_number(r,g_font_large,p.score,x4+pad,by+bh/4,HUD_ACCENT);
    char kb[64]; SDL_snprintf(kb,sizeof(kb),"KILLS:%d LVL:%d",p.kills,p.level);
    hud_text(r,g_font_small,kb,x4+pad,by+bh*2/3,HUD_GRAY);
}

static void draw_weapon_hud(SDL_Renderer* r, const Player& p) {
    int ww=g_sw/3, wh=g_sh*2/3;
    int wx=g_sw/2-ww/2, wy=g_sh/2-wh/2;
    hud_rect(r,wx,wy,ww,wh,{5,3,10,225});
    hud_rect_out(r,wx,wy,ww,wh,HUD_ACCENT);
    hud_text_center(r,g_font_medium,"[ WEAPONS ]",g_sw/2,wy+8,HUD_ACCENT);

    int lh=wh/17;
    for(int i=0;i<MAX_WEAPONS;i++){
        const Weapon& w=p.weapons[i];
        bool cur=(i==p.currentWeapon);
        int iy=wy+30+i*lh;
        if(cur) hud_rect(r,wx+4,iy-2,ww-8,lh,{60,40,8,180});
        Color nc=w.owned?(cur?HUD_ACCENT:HUD_WHITE):HUD_GRAY;
        char buf[64];
        if(w.owned&&w.maxAmmo>0) SDL_snprintf(buf,sizeof(buf),"%d %s [%d]",i+1,weapon_names[i],w.ammo);
        else if(w.owned)         SDL_snprintf(buf,sizeof(buf),"%d %s",i+1,weapon_names[i]);
        else                     SDL_snprintf(buf,sizeof(buf),"%d ---",i+1);
        hud_text(r,g_font_small,buf,wx+12,iy,nc);
    }
    hud_text_center(r,g_font_small,"[E] SWAP  [WHEEL/1-9] SELECT",g_sw/2,wy+wh-20,HUD_GRAY);
}

static void draw_minimap(SDL_Renderer* r, const Player& p) {
    int cell=std::max(4,g_sw/160);
    int draw=std::min(32,g_sw/(cell*2));
    int mw=draw*cell, mh=draw*cell;
    int mx=g_sw/2-mw/2, my=20;

    hud_rect(r,mx-3,my-3,mw+6,mh+6,{5,3,10,220});
    hud_rect_out(r,mx-3,my-3,mw+6,mh+6,HUD_ACCENT);

    int camX=(int)p.pos.x-draw/2;
    int camY=(int)p.pos.y-draw/2;

    for(int y=0;y<draw;y++)
    for(int x=0;x<draw;x++){
        int wx=camX+x,wy=camY+y;
        if(!world_in_bounds(wx,wy)) continue;
        Tile& t=g_world.map[wy][wx];
        Color tc={0,0,0,0};
        switch(t.type){
        case TILE_WALL:  tc={55,60,78,200}; break;
        case TILE_EMPTY: tc={28,28,38,200}; break;
        case TILE_DOOR:  tc={100,80,20,200};break;
        case TILE_EXIT:  tc={20,180,20,200};break;
        }
        if(tc.a==0) continue;
        SDL_Rect rc={mx+x*cell,my+y*cell,cell-1,cell-1};
        SDL_SetRenderDrawColor(r,tc.r,tc.g,tc.b,tc.a);
        SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
        SDL_RenderFillRect(r,&rc);
    }

    for(int i=0;i<g_entityCount;i++){
        Entity& e=g_entities[i];
        if(!e.active||e.state==ES_DEAD) continue;
        int ex=(int)e.pos.x-camX, ey=(int)e.pos.y-camY;
        if(ex<0||ex>=draw||ey<0||ey>=draw) continue;
        Color ec=e.type==ENT_ENEMY?HUD_RED:HUD_GREEN;
        SDL_Rect er={mx+ex*cell+1,my+ey*cell+1,cell-2,cell-2};
        SDL_SetRenderDrawColor(r,ec.r,ec.g,ec.b,220);
        SDL_RenderFillRect(r,&er);
    }

    int px=(int)p.pos.x-camX, py=(int)p.pos.y-camY;
    SDL_SetRenderDrawColor(r,255,255,80,255);
    SDL_Rect pr={mx+px*cell-1,my+py*cell-1,cell+2,cell+2};
    SDL_RenderFillRect(r,&pr);
    SDL_SetRenderDrawColor(r,255,200,0,255);
    SDL_RenderDrawLine(r,
        mx+px*cell+cell/2,my+py*cell+cell/2,
        mx+px*cell+cell/2+(int)(p.dir.x*cell*2),
        my+py*cell+cell/2+(int)(p.dir.y*cell*2));

    char lb[32]; SDL_snprintf(lb,sizeof(lb),"LEVEL %d",p.level);
    hud_text_center(r,g_font_small,lb,g_sw/2,my+mh+4,HUD_ACCENT);
}

static void draw_debug(SDL_Renderer* r, const Player& p, double fps, double ft) {
    int pw=g_sw*3/8, lh=g_sh/42;
    hud_rect(r,0,0,pw,lh*17,{5,3,10,215});
    hud_rect_out(r,0,0,pw,lh*17,HUD_ACCENT);

    int py2=4;
    auto line=[&](const char* s, const Color& c=HUD_WHITE){
        hud_text(r,g_font_small,s,8,py2,c); py2+=lh;
    };
    char buf[128];
    SDL_snprintf(buf,sizeof(buf),"Null Zone %s  [F3 hide]",NZ_VERSION);
    line(buf,HUD_ACCENT);
    SDL_snprintf(buf,sizeof(buf),"FPS: %.1f  Frame: %.2fms",fps,ft*1000.0);
    line(buf,fps>=55?HUD_GREEN:fps>=30?HUD_ACCENT:HUD_RED);
    SDL_snprintf(buf,sizeof(buf),"Resolution: %dx%d",g_sw,g_sh);
    line(buf);
    line("");
    SDL_snprintf(buf,sizeof(buf),"XY: %.2f / %.2f",p.pos.x,p.pos.y);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"Angle: %.1f deg",p.angle*180.0/M_PI);
    line(buf);
    line("");
    SDL_snprintf(buf,sizeof(buf),"Level: %d  Seed: %u",p.level,g_world.seed);
    line(buf);
    SDL_snprintf(buf,sizeof(buf),"Entities: %d  Aggro: %d",g_entityCount,entities_count_aggro());
    line(buf,HUD_ORANGE);
    SDL_snprintf(buf,sizeof(buf),"Frozen: %d  Visible: %d",entities_count_frozen(),entities_count_aggro());
    line(buf,HUD_GRAY);
    int ar=0;
    for(int i=0;i<g_world.roomCount;i++) if(g_world.rooms[i].state==ROOM_VISIBLE) ar++;
    SDL_snprintf(buf,sizeof(buf),"Active rooms: %d / %d",ar,g_world.roomCount);
    line(buf);
    unsigned mem=(unsigned)((sizeof(g_world)+sizeof(g_entities)+sizeof(g_textures))/1024);
    SDL_snprintf(buf,sizeof(buf),"Memory: %u KB",mem);
    line(buf);
    line("");
    SDL_snprintf(buf,sizeof(buf),"Weapon: %s",weapon_names[p.currentWeapon]);
    line(buf,HUD_ACCENT);
    SDL_snprintf(buf,sizeof(buf),"Ammo: %d  HP: %d  AR: %d",p.weapons[p.currentWeapon].ammo,p.health,p.armor);
    line(buf,HUD_GREEN);
}

static void draw_level_complete(SDL_Renderer* r, const Player& p) {
    int bw=g_sw/2, bh=g_sh/5;
    int bx=g_sw/2-bw/2, by=g_sh/2-bh/2;
    hud_rect(r,bx,by,bw,bh,{5,3,10,235});
    hud_rect_out(r,bx,by,bw,bh,HUD_ACCENT);
    hud_text_center(r,g_font_large,"LEVEL COMPLETE!",g_sw/2,by+bh/6,HUD_ACCENT);
    char buf[64]; SDL_snprintf(buf,sizeof(buf),"KILLS: %d    SCORE: %d",p.kills,p.score);
    hud_text_center(r,g_font_medium,buf,g_sw/2,by+bh/2,HUD_WHITE);
    hud_text_center(r,g_font_small,"Press ENTER to continue",g_sw/2,by+bh*3/4,HUD_GRAY);
}

static void draw_dead(SDL_Renderer* r, const Player& p) {
    hud_rect(r,0,0,g_sw,g_sh,{80,0,0,110});
    int bw=g_sw*2/5, bh=g_sh/5;
    int bx=g_sw/2-bw/2, by=g_sh/2-bh/2;
    hud_rect(r,bx,by,bw,bh,{20,3,3,240});
    hud_rect_out(r,bx,by,bw,bh,HUD_RED);
    hud_text_center(r,g_font_large,"YOU DIED",g_sw/2,by+bh/6,HUD_RED);
    char buf[64]; SDL_snprintf(buf,sizeof(buf),"Score:%d  Kills:%d  Level:%d",p.score,p.kills,p.level);
    hud_text_center(r,g_font_small,buf,g_sw/2,by+bh/2,HUD_GRAY);
    hud_text_center(r,g_font_small,"R - Restart    ESC - Menu",g_sw/2,by+bh*3/4,HUD_WHITE);
}

inline void hud_draw(SDL_Renderer* r, const Player& p, double fps, double ft, GameState state) {
    SDL_GetRendererOutputSize(r,&g_sw,&g_sh);

    if(state==GS_PLAYING||state==GS_PAUSED){
        draw_hud_bottom(r,p);
        if(g_show_map) draw_minimap(r,p);
        if(action_wpnhud()) draw_weapon_hud(r,p);
        if(g_show_debug) draw_debug(r,p,fps,ft);
    }
    if(state==GS_LEVEL_COMPLETE) draw_level_complete(r,p);
    if(state==GS_DEAD)           draw_dead(r,p);
}

inline void hud_toggle_debug() { g_show_debug=!g_show_debug; }
inline void hud_toggle_map()   { g_show_map=!g_show_map;     }
