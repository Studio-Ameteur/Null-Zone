#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include "common.h"

struct SpriteSheet {
    SDL_Texture* tex;
    int frameW;
    int frameH;
    int cols;
    int rows;
    int totalFrames;
};

static SpriteSheet g_weapon_sheets[MAX_WEAPONS] = {};
static SpriteSheet g_enemy_sheets[15]           = {};
static SDL_Renderer* g_spr_rend                 = nullptr;

static const char* weapon_files[MAX_WEAPONS] = {
    nullptr,
    "assets/sprites/weapons/PIST1.png",
    "assets/sprites/weapons/Sgun.png",
    "assets/sprites/weapons/MCGUN1.png",
    "assets/sprites/weapons/PIST2.png",
    nullptr,
    nullptr,
    "assets/sprites/weapons/Sgun.png",
    nullptr,
    nullptr,
    nullptr,
    "assets/sprites/weapons/MCGUN1.png",
    nullptr,
    nullptr,
    "assets/sprites/weapons/PICK1.png"
};

struct EnemySpriteInfo {
    const char* file;
    int frameW;
    int frameH;
};

static const EnemySpriteInfo enemy_info[15] = {
    {"assets/sprites/enemies/Nosferatu_Float.png", 48, 72},
    {"assets/sprites/enemies/Zombie_Walk.png",     48, 72},
    {"assets/sprites/enemies/Zombie_Walk.png",     48, 72},
    {"assets/sprites/enemies/Nosferatu_Float.png", 48, 72},
    {"assets/sprites/enemies/Zombie_Walk.png",     48, 72},
    {"assets/sprites/enemies/Zombie_Walk.png",     48, 72},
    {"assets/sprites/enemies/Nosferatu_Walk.png",  48, 72},
    {"assets/sprites/enemies/Zombie_Walk.png",     48, 72},
    {"assets/sprites/enemies/Nosferatu_Walk.png",  48, 72},
    {"assets/sprites/enemies/Angel_Walk.png",      48, 64},
    {"assets/sprites/enemies/Angel_Walk.png",      48, 64},
    {"assets/sprites/enemies/Nosferatu_Float.png", 48, 72},
    {"assets/sprites/enemies/BossesA.png",         48, 72},
    {"assets/sprites/enemies/BossesB.png",         48, 72},
    {"assets/sprites/enemies/Grotesque_Boss.png",  96,128}
};

static SpriteSheet load_sheet(SDL_Renderer* r, const char* path, int fw, int fh) {
    SpriteSheet s = {};
    if(!path) return s;
    SDL_Surface* surf = IMG_Load(path);
    if(!surf) return s;
    s.tex        = SDL_CreateTextureFromSurface(r, surf);
    s.frameW     = fw;
    s.frameH     = fh;
    s.cols       = surf->w / fw;
    s.rows       = surf->h / fh;
    s.totalFrames= s.cols * s.rows;
    if(s.cols<1) s.cols=1;
    if(s.rows<1) s.rows=1;
    if(s.totalFrames<1) s.totalFrames=1;
    SDL_FreeSurface(surf);
    return s;
}

inline void sprites_init(SDL_Renderer* rend) {
    g_spr_rend = rend;
    IMG_Init(IMG_INIT_PNG|IMG_INIT_JPG);

    for(int i=0;i<MAX_WEAPONS;i++){
        if(!weapon_files[i]) continue;
        SDL_Surface* s=IMG_Load(weapon_files[i]);
        if(!s) continue;
        g_weapon_sheets[i].tex        = SDL_CreateTextureFromSurface(rend,s);
        g_weapon_sheets[i].frameW     = s->w;
        g_weapon_sheets[i].frameH     = s->h;
        g_weapon_sheets[i].cols       = 1;
        g_weapon_sheets[i].rows       = 1;
        g_weapon_sheets[i].totalFrames= 1;
        SDL_FreeSurface(s);
    }

    for(int i=0;i<15;i++){
        bool reused=false;
        for(int j=0;j<i;j++){
            if(enemy_info[j].file &&
               strcmp(enemy_info[i].file, enemy_info[j].file)==0 &&
               enemy_info[i].frameW==enemy_info[j].frameW &&
               enemy_info[i].frameH==enemy_info[j].frameH) {
                g_enemy_sheets[i]=g_enemy_sheets[j];
                reused=true; break;
            }
        }
        if(!reused)
            g_enemy_sheets[i]=load_sheet(rend,
                enemy_info[i].file,
                enemy_info[i].frameW,
                enemy_info[i].frameH);
    }
}

inline void sprites_shutdown() {
    for(int i=0;i<MAX_WEAPONS;i++)
        if(g_weapon_sheets[i].tex){SDL_DestroyTexture(g_weapon_sheets[i].tex);g_weapon_sheets[i].tex=nullptr;}

    for(int i=0;i<15;i++){
        bool shared=false;
        for(int j=0;j<i;j++)
            if(g_enemy_sheets[i].tex==g_enemy_sheets[j].tex){shared=true;break;}
        if(!shared&&g_enemy_sheets[i].tex)
            SDL_DestroyTexture(g_enemy_sheets[i].tex);
        g_enemy_sheets[i].tex=nullptr;
    }
    IMG_Quit();
}

inline bool sprites_draw_weapon(SDL_Renderer* r, int wi,
                                 double bobAmt, double bobSide,
                                 int sw, int sh) {
    SpriteSheet& s=g_weapon_sheets[wi];
    if(!s.tex) return false;

    int dh=sh*2/5;
    int dw=(int)((double)s.frameW/s.frameH*dh);
    int dx=sw/2-dw/2+(int)(bobSide*sw*0.02);
    int dy=sh-dh+(int)(std::abs(bobAmt)*sh*0.05);

    SDL_Rect dst={dx,dy,dw,dh};
    SDL_RenderCopy(r,s.tex,nullptr,&dst);
    return true;
}

inline bool sprites_draw_enemy(SDL_Renderer* r, int enemyType,
                                int animFrame, int animState,
                                int screenX, int screenY,
                                int size, double distF) {
    SpriteSheet& s=g_enemy_sheets[enemyType];
    if(!s.tex) return false;

    int frame=animFrame%s.totalFrames;
    int col=frame%s.cols;
    int row=0;

    if(s.rows>1){
        switch(animState){
        case 0: row=0; break;
        case 1: row=std::min(1,s.rows-1); break;
        case 2: row=std::min(2,s.rows-1); break;
        case 3: row=std::min(3,s.rows-1); break;
        default:row=0; break;
        }
    }

    SDL_Rect src={col*s.frameW, row*s.frameH, s.frameW, s.frameH};
    SDL_Rect dst={screenX-size/2, screenY-size/2, size, size};

    uint8_t mod=(uint8_t)std::min(255.0,distF*255.0);
    SDL_SetTextureColorMod(s.tex,mod,mod,mod);
    SDL_SetTextureBlendMode(s.tex,SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(r,s.tex,&src,&dst);
    SDL_SetTextureColorMod(s.tex,255,255,255);
    return true;
}

inline int enemy_anim_state(int entityState) {
    switch(entityState){
    case ES_IDLE:   return 0;
    case ES_ALERT:  return 0;
    case ES_CHASE:  return 1;
    case ES_ATTACK: return 2;
    case ES_DEAD:   return 3;
    default:        return 0;
    }
}
