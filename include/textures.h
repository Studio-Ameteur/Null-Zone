#pragma once
#include "common.h"

struct Texture {
    Color pixels[TEX_SIZE][TEX_SIZE];
};

static Texture g_textures[NUM_TEXTURES];

static inline uint8_t c8(int v) {
    return (uint8_t)(v<0?0:v>255?255:v);
}

static inline void px(Texture& t, int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if(x<0||x>=TEX_SIZE||y<0||y>=TEX_SIZE) return;
    t.pixels[y][x] = {r,g,b,255};
}

static inline void fill(Texture& t, uint8_t r, uint8_t g, uint8_t b) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++)
        t.pixels[y][x]={r,g,b,255};
}

static inline void hline(Texture& t, int y, uint8_t r, uint8_t g, uint8_t b) {
    for(int x=0;x<TEX_SIZE;x++) px(t,x,y,r,g,b);
}

static inline void vline(Texture& t, int x, uint8_t r, uint8_t g, uint8_t b) {
    for(int y=0;y<TEX_SIZE;y++) px(t,x,y,r,g,b);
}

static inline void rect(Texture& t, int x0,int y0,int x1,int y1, uint8_t r,uint8_t g,uint8_t b) {
    for(int y=y0;y<=y1;y++)
    for(int x=x0;x<=x1;x++)
        px(t,x,y,r,g,b);
}

static int _px_rng(int x, int y, int seed) {
    int n = x*374761393 + y*668265263 + seed*2246822519;
    n = (n^(n>>13))*1274126177;
    return n^(n>>16);
}

static inline uint8_t _var(int x, int y, int seed, int range) {
    return (uint8_t)(abs(_px_rng(x,y,seed)) % range);
}

static void tex_cold_panel(Texture& t) {
    fill(t, 55,65,80);
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v = _var(x,y,1,12);
        t.pixels[y][x] = {c8(52+v),c8(62+v),c8(78+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) { hline(t,y,30,38,50); if(y+1<TEX_SIZE) hline(t,y+1,22,28,40); }
    for(int x=0;x<TEX_SIZE;x+=16) { vline(t,x,30,38,50); if(x+1<TEX_SIZE) vline(t,x+1,22,28,40); }
    for(int y=2;y<TEX_SIZE;y+=16)
    for(int x=2;x<TEX_SIZE;x+=16) {
        px(t,x,y,80,95,115); px(t,x+1,y,70,85,105);
        px(t,x,y+1,70,85,105); px(t,x+1,y+1,60,75,95);
    }
}

static void tex_cold_panel_damage(Texture& t) {
    fill(t, 50,60,75);
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,2,10);
        t.pixels[y][x]={c8(48+v),c8(58+v),c8(72+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) { hline(t,y,28,35,48); if(y+1<TEX_SIZE) hline(t,y+1,20,26,38); }
    for(int x=0;x<TEX_SIZE;x+=16) { vline(t,x,28,35,48); if(x+1<TEX_SIZE) vline(t,x+1,20,26,38); }
    rect(t,5,18,25,19,20,24,32);
    rect(t,8,17,9,22,20,24,32);
    rect(t,35,40,55,41,20,24,32);
    rect(t,52,38,53,44,20,24,32);
    rect(t,15,48,16,56,20,24,32);
    rect(t,14,55,22,56,20,24,32);
}

static void tex_cold_vent(Texture& t) {
    fill(t,35,42,55);
    for(int y=0;y<TEX_SIZE;y+=8) {
        rect(t,0,y,63,y+1,22,28,40);
        rect(t,0,y+2,63,y+5,42,52,68);
        rect(t,0,y+6,63,y+7,30,38,50);
    }
    for(int x=0;x<TEX_SIZE;x+=4) {
        rect(t,x,0,x+1,63,25,32,44);
    }
    rect(t,0,0,63,1,60,75,95);
    rect(t,0,62,63,63,60,75,95);
}

static void tex_cold_energy(Texture& t) {
    fill(t,20,25,40);
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,4,8);
        t.pixels[y][x]={c8(18+v),c8(22+v),c8(38+v),255};
    }
    for(int y=4;y<TEX_SIZE;y+=8) rect(t,4,y,59,y+1,40,120,200);
    for(int x=4;x<TEX_SIZE;x+=12) rect(t,x,4,x+1,59,40,120,200);
    rect(t,20,20,43,43,30,90,160);
    rect(t,24,24,39,39,50,150,220);
    rect(t,28,28,35,35,80,180,255);
    rect(t,30,30,33,33,150,220,255);
}

static void tex_cold_pipe(Texture& t) {
    fill(t,40,50,65);
    rect(t,0,8,63,23,55,68,85);
    rect(t,0,9,63,22,65,80,100);
    rect(t,0,10,63,21,72,88,110);
    rect(t,0,11,63,12,82,100,125);
    rect(t,0,8,63,8,30,38,50);
    rect(t,0,23,63,23,30,38,50);
    for(int x=0;x<TEX_SIZE;x+=12) {
        rect(t,x,8,x+1,23,40,50,65);
        rect(t,x+2,9,x+3,22,90,110,135);
    }
    rect(t,0,36,63,51,48,58,72);
    rect(t,0,37,63,50,56,68,84);
    rect(t,0,38,63,49,62,76,95);
    rect(t,0,36,63,36,28,35,46);
    rect(t,0,51,63,51,28,35,46);
}

static void tex_cold_floor(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        bool checker = ((x/8)+(y/8))%2==0;
        uint8_t v=_var(x,y,5,8);
        if(checker) t.pixels[y][x]={c8(45+v),c8(55+v),c8(70+v),255};
        else        t.pixels[y][x]={c8(35+v),c8(42+v),c8(55+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=8) hline(t,y,22,28,38);
    for(int x=0;x<TEX_SIZE;x+=8) vline(t,x,22,28,38);
}

static void tex_cold_ceil(Texture& t) {
    fill(t,28,35,48);
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,6,6);
        t.pixels[y][x]={c8(26+v),c8(32+v),c8(44+v),255};
    }
    rect(t,8,8,55,18,55,68,88);
    rect(t,10,9,53,17,65,80,105);
    rect(t,12,10,51,16,200,210,220);
    rect(t,14,11,49,15,230,235,240);
    rect(t,8,8,55,8,40,50,65);
    rect(t,8,18,55,18,40,50,65);
    rect(t,8,8,8,18,40,50,65);
    rect(t,55,8,55,18,40,50,65);
}

static void tex_ind_metal(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,10,14);
        t.pixels[y][x]={c8(75+v),c8(60+v),c8(40+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) { hline(t,y,40,30,18); if(y+1<TEX_SIZE) hline(t,y+1,30,22,12); }
    for(int x=0;x<TEX_SIZE;x+=16) { vline(t,x,40,30,18); if(x+1<TEX_SIZE) vline(t,x+1,30,22,12); }
    for(int y=2;y<TEX_SIZE;y+=16)
    for(int x=2;x<TEX_SIZE;x+=16) {
        px(t,x,y,110,90,65); px(t,x+1,y,95,78,55);
        px(t,x,y+1,95,78,55); px(t,x+1,y+1,80,65,45);
    }
}

static void tex_ind_rust(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,11,16);
        bool rust = (_var(x,y,77,3)==0);
        if(rust) t.pixels[y][x]={c8(140+v),c8(60+v),c8(20+v),255};
        else     t.pixels[y][x]={c8(80+v), c8(65+v),c8(45+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) { hline(t,y,35,25,15); if(y+1<TEX_SIZE) hline(t,y+1,25,18,10); }
    for(int x=0;x<TEX_SIZE;x+=16) { vline(t,x,35,25,15); if(x+1<TEX_SIZE) vline(t,x+1,25,18,10); }
    rect(t,3,19,5,26,120,50,15);
    rect(t,40,3,43,8,110,45,12);
    rect(t,55,35,58,42,130,55,18);
}

static void tex_ind_warn_yellow(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        int stripe=(x+y)/8;
        uint8_t v=_var(x,y,12,10);
        if(stripe%2==0) t.pixels[y][x]={c8(220+v),c8(180+v),20,255};
        else            t.pixels[y][x]={c8(25+v), c8(20+v), c8(15+v),255};
    }
    hline(t,0,180,140,10); hline(t,1,160,120,8);
    hline(t,62,180,140,10); hline(t,63,160,120,8);
    vline(t,0,180,140,10); vline(t,1,160,120,8);
    vline(t,62,180,140,10); vline(t,63,160,120,8);
}

static void tex_ind_warn_red(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        int stripe=(x+y)/8;
        uint8_t v=_var(x,y,13,10);
        if(stripe%2==0) t.pixels[y][x]={c8(220+v),c8(20+v),c8(20+v),255};
        else            t.pixels[y][x]={c8(200+v),c8(200+v),c8(200+v),255};
    }
    hline(t,0,180,15,15); hline(t,63,180,15,15);
    vline(t,0,180,15,15); vline(t,63,180,15,15);
}

static void tex_ind_grate(Texture& t) {
    fill(t,20,16,10);
    for(int y=0;y<TEX_SIZE;y+=6) {
        rect(t,0,y,63,y+1,80,65,42);
        rect(t,0,y+2,63,y+3,65,52,34);
    }
    for(int x=0;x<TEX_SIZE;x+=6) {
        rect(t,x,0,x+1,63,80,65,42);
    }
}

static void tex_porthole(Texture& t) {
    fill(t,50,62,78);
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        int cx=x-32,cy=y-32;
        int d2=cx*cx+cy*cy;
        if(d2<576) {
            if(_var(x,y,99,20)<2) t.pixels[y][x]={240,240,255,255};
            else t.pixels[y][x]={5,5,c8(15+abs(cx)+abs(cy)/2),255};
        } else if(d2<676) {
            t.pixels[y][x]={40,50,65,255};
        } else {
            uint8_t v=_var(x,y,7,10);
            t.pixels[y][x]={c8(52+v),c8(62+v),c8(78+v),255};
        }
    }
    rect(t,28,0,35,6,60,75,95);
    rect(t,28,57,35,63,60,75,95);
    rect(t,0,28,6,35,60,75,95);
    rect(t,57,28,63,35,60,75,95);
}

static void tex_ind_energy_red(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,15,8);
        t.pixels[y][x]={c8(18+v),c8(10+v),c8(10+v),255};
    }
    for(int y=4;y<TEX_SIZE;y+=8) rect(t,4,y,59,y+1,200,30,20);
    for(int x=4;x<TEX_SIZE;x+=12) rect(t,x,4,x+1,59,200,30,20);
    rect(t,20,20,43,43,150,20,15);
    rect(t,24,24,39,39,200,30,20);
    rect(t,28,28,35,35,230,60,40);
    rect(t,30,30,33,33,255,100,60);
}

static void tex_ind_floor(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,16,12);
        t.pixels[y][x]={c8(65+v),c8(52+v),c8(35+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) hline(t,y,35,28,18);
    for(int x=0;x<TEX_SIZE;x+=16) vline(t,x,35,28,18);
    for(int y=1;y<TEX_SIZE;y+=16)
    for(int x=1;x<TEX_SIZE;x+=16)
        px(t,x,y,100,82,58);
}

static void tex_cold_grate_floor(Texture& t) {
    fill(t,15,18,25);
    for(int y=0;y<TEX_SIZE;y+=6) {
        rect(t,0,y,63,y+1,55,68,88);
        rect(t,0,y+2,63,y+3,40,50,65);
    }
    for(int x=0;x<TEX_SIZE;x+=6) {
        rect(t,x,0,x+1,63,55,68,88);
    }
}

static void tex_ind_ceil(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,20,8);
        t.pixels[y][x]={c8(38+v),c8(30+v),c8(20+v),255};
    }
    for(int x=0;x<TEX_SIZE;x+=8) rect(t,x,0,x+3,63,28,22,14);
    rect(t,10,20,53,30,60,75,95);
    rect(t,12,21,51,29,180,185,190);
    rect(t,14,22,49,28,220,222,225);
}

static void tex_cold_ceil_vent(Texture& t) {
    fill(t,25,32,44);
    for(int y=0;y<TEX_SIZE;y+=10) {
        rect(t,0,y,63,y+1,40,50,65);
        rect(t,0,y+2,63,y+7,35,44,58);
    }
    for(int x=0;x<TEX_SIZE;x+=10) rect(t,x,0,x+1,63,40,50,65);
    rect(t,0,0,63,1,60,75,95);
    rect(t,0,62,63,63,60,75,95);
    rect(t,0,0,1,63,60,75,95);
    rect(t,62,0,63,63,60,75,95);
}

static void tex_ind_ceil_pipes(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,22,6);
        t.pixels[y][x]={c8(35+v),c8(28+v),c8(18+v),255};
    }
    rect(t,6,0,14,63,70,56,38);
    rect(t,8,0,12,63,85,68,46);
    rect(t,10,0,10,63,100,80,55);
    rect(t,6,0,14,0,45,36,24);
    rect(t,6,63,14,63,45,36,24);
    rect(t,30,0,38,63,65,52,35);
    rect(t,32,0,36,63,80,64,43);
    rect(t,34,0,34,63,95,76,51);
    rect(t,50,0,58,63,68,54,36);
    rect(t,52,0,56,63,82,66,44);
    rect(t,0,28,63,30,20,16,10);
    rect(t,0,28,63,28,40,32,20);
}

static void tex_cold_floor_marking(Texture& t) {
    for(int y=0;y<TEX_SIZE;y++)
    for(int x=0;x<TEX_SIZE;x++) {
        uint8_t v=_var(x,y,23,8);
        t.pixels[y][x]={c8(42+v),c8(52+v),c8(66+v),255};
    }
    for(int y=0;y<TEX_SIZE;y+=16) hline(t,y,22,28,38);
    for(int x=0;x<TEX_SIZE;x+=16) vline(t,x,22,28,38);
    rect(t,28,8,35,55,40,120,200);
    rect(t,29,9,34,54,50,140,220);
    rect(t,16,28,47,35,40,120,200);
    rect(t,17,29,46,34,50,140,220);
}

inline void textures_init() {
    tex_cold_panel        (g_textures[0]);
    tex_cold_panel_damage (g_textures[1]);
    tex_cold_vent         (g_textures[2]);
    tex_cold_energy       (g_textures[3]);
    tex_cold_pipe         (g_textures[4]);
    tex_porthole          (g_textures[5]);
    tex_ind_metal         (g_textures[6]);
    tex_ind_rust          (g_textures[7]);
    tex_ind_warn_yellow   (g_textures[8]);
    tex_ind_warn_red      (g_textures[9]);
    tex_ind_grate         (g_textures[10]);
    tex_ind_energy_red    (g_textures[11]);
    tex_cold_floor        (g_textures[12]);
    tex_cold_grate_floor  (g_textures[13]);
    tex_cold_floor_marking(g_textures[14]);
    tex_ind_floor         (g_textures[15]);
    tex_cold_ceil         (g_textures[16]);
    tex_cold_ceil_vent    (g_textures[17]);
    tex_ind_ceil          (g_textures[18]);
    tex_ind_ceil_pipes    (g_textures[19]);
    tex_cold_panel        (g_textures[20]);
    tex_ind_metal         (g_textures[21]);
    tex_cold_energy       (g_textures[22]);
}

inline const Color& tex_sample(int idx, int x, int y) {
    x = ((x%TEX_SIZE)+TEX_SIZE)%TEX_SIZE;
    y = ((y%TEX_SIZE)+TEX_SIZE)%TEX_SIZE;
    return g_textures[idx].pixels[y][x];
}
