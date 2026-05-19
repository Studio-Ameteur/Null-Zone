#pragma once
#include <SDL.h>
#include <SDL_image.h>
#include "common.h"

static const int NUM_WALL_TEXTURES  = 50;
static const int NUM_FLOOR_TEXTURES = 7;

struct Texture {
    Color pixels[TEX_SIZE][TEX_SIZE];
};

static Texture      g_textures[NUM_TEXTURES];
static SDL_Texture* g_wall_tex[NUM_WALL_TEXTURES]   = {};
static SDL_Texture* g_floor_tex[NUM_FLOOR_TEXTURES] = {};
static SDL_Renderer* g_tex_rend = nullptr;

static const char* floor_paths[NUM_FLOOR_TEXTURES] = {
    "assets/textures/floors/cobblestone spritesheet.png",
    "assets/textures/floors/stone square spritesheet.png",
    "assets/textures/floors/wood spritesheet.png",
    "assets/textures/floors/carpet spritesheet.png",
    "assets/textures/floors/chckerboard spritesheet.png",
    "assets/textures/floors/pebbles spritesheet.png",
    "assets/textures/floors/black and white.png",
};

static inline uint8_t c8(int v) {
    return (uint8_t)(v<0?0:v>255?255:v);
}

static inline void px(Texture& t, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if(x<0||x>=TEX_SIZE||y<0||y>=TEX_SIZE) return;
    t.pixels[y][x]={r,g,b,255};
}

static inline void hline(Texture& t, int y, uint8_t r, uint8_t g, uint8_t b) {
    for(int x=0;x<TEX_SIZE;x++) px(t,x,y,r,g,b);
}

static inline void vline(Texture& t, int x, uint8_t r, uint8_t g, uint8_t b) {
    for(int y=0;y<TEX_SIZE;y++) px(t,x,y,r,g,b);
}

static int _prng(int x, int y, int s) {
    int n=x*374761393+y*668265263+s*2246822519;
    n=(n^(n>>13))*1274126177; return n^(n>>16);
}

static inline uint8_t _var(int x, int y, int s, int r) {
    return (uint8_t)(abs(_prng(x,y,s))%r);
}

static void gen_fallback(Texture& t, int idx) {
    uint8_t br,bg,bb;
    if     (idx<16) {br=55;bg=65;bb=80;}
    else if(idx<32) {br=75;bg=60;bb=40;}
    else            {br=60;bg=55;bb=70;}
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++){
        uint8_t v=_var(x,y,idx,16);
        t.pixels[y][x]={c8(br+v),c8(bg+v),c8(bb+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) hline(t,y,br/2,bg/2,bb/2);
    for(int x=0;x<TEX_SIZE;x+=16) vline(t,x,br/2,bg/2,bb/2);
}

static void gen_fallback_floor(Texture& t, int idx) {
    static const uint8_t cols[7][3]={
        {60,55,50},{50,50,55},{80,60,40},
        {60,40,40},{55,55,55},{50,55,50},{40,40,40}
    };
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++){
        uint8_t v=_var(x,y,idx+100,12);
        t.pixels[y][x]={c8(cols[idx][0]+v),c8(cols[idx][1]+v),c8(cols[idx][2]+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=8) hline(t,y,cols[idx][0]/2,cols[idx][1]/2,cols[idx][2]/2);
    for(int x=0;x<TEX_SIZE;x+=8) vline(t,x,cols[idx][0]/2,cols[idx][1]/2,cols[idx][2]/2);
}

static void png_to_pixels(SDL_Renderer* r, const char* path, Texture& out) {
    SDL_Surface* s=IMG_Load(path);
    if(!s) return;
    SDL_Surface* c=SDL_ConvertSurfaceFormat(s,SDL_PIXELFORMAT_ARGB8888,0);
    SDL_FreeSurface(s);
    if(!c) return;
    double sx=(double)c->w/TEX_SIZE;
    double sy=(double)c->h/TEX_SIZE;
    Uint32* pix=(Uint32*)c->pixels;
    int pitch=c->pitch/4;
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++){
        int px2=std::min((int)(x*sx),c->w-1);
        int py2=std::min((int)(y*sy),c->h-1);
        Uint32 p=pix[py2*pitch+px2];
        out.pixels[y][x]={(uint8_t)((p>>16)&0xFF),(uint8_t)((p>>8)&0xFF),(uint8_t)(p&0xFF),255};
    }
    SDL_FreeSurface(c);
}

static SDL_Texture* load_tex(SDL_Renderer* r, const char* path) {
    SDL_Surface* s=IMG_Load(path);
    if(!s) return nullptr;
    SDL_Texture* t=SDL_CreateTextureFromSurface(r,s);
    SDL_FreeSurface(s);
    return t;
}

inline void textures_init() {
    for(int i=0;i<NUM_TEXTURES;i++) gen_fallback(g_textures[i],i);
}

inline void textures_init_renderer(SDL_Renderer* rend) {
    g_tex_rend=rend;

    for(int i=0;i<NUM_WALL_TEXTURES;i++){
        char path[128];
        SDL_snprintf(path,sizeof(path),"assets/textures/walls/Asset %d.png",i+1);
        g_wall_tex[i]=load_tex(rend,path);
        if(i<NUM_TEXTURES){
            if(g_wall_tex[i]) png_to_pixels(rend,path,g_textures[i]);
            else              gen_fallback(g_textures[i],i);
        }
    }

    for(int i=0;i<NUM_FLOOR_TEXTURES;i++){
        g_floor_tex[i]=load_tex(rend,floor_paths[i]);
        int ti=NUM_WALL_TEXTURES+i;
        if(ti<NUM_TEXTURES){
            if(g_floor_tex[i]) png_to_pixels(rend,floor_paths[i],g_textures[ti]);
            else               gen_fallback_floor(g_textures[ti],i);
        }
    }
}

inline void textures_shutdown() {
    for(int i=0;i<NUM_WALL_TEXTURES;i++)
        if(g_wall_tex[i]){SDL_DestroyTexture(g_wall_tex[i]);g_wall_tex[i]=nullptr;}
    for(int i=0;i<NUM_FLOOR_TEXTURES;i++)
        if(g_floor_tex[i]){SDL_DestroyTexture(g_floor_tex[i]);g_floor_tex[i]=nullptr;}
}

inline const Color& tex_sample(int idx, int x, int y) {
    idx=((idx%NUM_TEXTURES)+NUM_TEXTURES)%NUM_TEXTURES;
    x=((x%TEX_SIZE)+TEX_SIZE)%TEX_SIZE;
    y=((y%TEX_SIZE)+TEX_SIZE)%TEX_SIZE;
    return g_textures[idx].pixels[y][x];
}

inline SDL_Texture* tex_get_wall_sdl(int idx) {
    idx=((idx%NUM_WALL_TEXTURES)+NUM_WALL_TEXTURES)%NUM_WALL_TEXTURES;
    return g_wall_tex[idx];
}

inline SDL_Texture* tex_get_floor_sdl(int idx) {
    idx=((idx%NUM_FLOOR_TEXTURES)+NUM_FLOOR_TEXTURES)%NUM_FLOOR_TEXTURES;
    return g_floor_tex[idx];
}

inline int level_wall_tex(int level, uint32_t seed) {
    uint32_t h=seed^(uint32_t)(level*2654435761u);
    h^=h>>16; h*=0x45d9f3b; h^=h>>16;
    return (int)(h%NUM_WALL_TEXTURES);
}

inline int level_floor_tex(int level, uint32_t seed) {
    uint32_t h=(seed^(uint32_t)(level*1234567891u))*0x9e3779b9;
    h^=h>>16;
    return (int)(h%NUM_FLOOR_TEXTURES);
}
