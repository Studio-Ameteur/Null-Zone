#pragma once
#include "common.h"

struct Texture {
    Color pixels[TEX_SIZE][TEX_SIZE];
};

static Texture g_textures[NUM_TEXTURES];

static inline double _noise2(int x, int y, int seed) {
    int n = x + y*57 + seed*131;
    n = (n<<13) ^ n;
    return 1.0 - ((n*(n*n*15731+789221)+1376312589) & 0x7fffffff) / 1073741824.0;
}

static inline double _smooth(double x, double y, int seed) {
    int ix=(int)x, iy=(int)y;
    double fx=x-ix, fy=y-iy;
    fx=fx*fx*(3-2*fx);
    fy=fy*fy*(3-2*fy);
    return _noise2(ix,  iy,  seed)*(1-fx)*(1-fy)
          +_noise2(ix+1,iy,  seed)*fx*(1-fy)
          +_noise2(ix,  iy+1,seed)*(1-fx)*fy
          +_noise2(ix+1,iy+1,seed)*fx*fy;
}

static inline double _fractal(double x, double y, int seed, int oct) {
    double v=0, a=1, f=1, m=0;
    for (int i=0; i<oct; i++) {
        v += _smooth(x*f, y*f, seed+i*7)*a;
        m += a; a*=0.5; f*=2.0;
    }
    return v/m;
}

static inline uint8_t c8(double v) {
    return (uint8_t)std::min(255.0, std::max(0.0, v));
}

static void tex_metal_clean(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n = _fractal(x*.3, y*.3, 1, 3)*.12;
        bool h=(y%16==0||y%16==1), v=(x%16==0||x%16==1);
        double b = .45+n - (h||v ? .15 : 0);
        if ((y%16==2&&x%16>2&&x%16<14)||(x%16==2&&y%16>2&&y%16<14)) b+=.08;
        uint8_t c=c8(b*200+30);
        t.pixels[y][x]={c,c8(c+5),c8(c+10),255};
    }
}

static void tex_metal_rust(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.25,y*.25,2,4), r=_fractal(x*.18,y*.18,99,3);
        bool h=(y%16==0||y%16==1), v=(x%16==0||x%16==1);
        double b=.4+n*.1-(h||v?.1:0);
        t.pixels[y][x]={c8(b*180+r*80),c8(b*120+r*20),c8(b*100),255};
    }
}

static void tex_metal_rivets(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.4,y*.4,3,2)*.1;
        bool h=(y%16==0||y%16==1), v=(x%16==0||x%16==1);
        double b=.42+n-(h||v?.12:0);
        double dx=x%16-8, dy=y%16-8;
        if (sqrt(dx*dx+dy*dy)<3.0) b=.75+n*.3;
        uint8_t c=c8(b*210+20);
        t.pixels[y][x]={c,c8(c+3),c8(c+8),255};
    }
}

static void tex_concrete_clean(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.15,y*.15,4,5);
        uint8_t c=c8((.5+n*.25)*160+40);
        t.pixels[y][x]={c,c,c8(c-5),255};
    }
}

static void tex_concrete_cracks(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.15,y*.15,5,5), cr=_fractal(x*.5,y*.5,55,2);
        double b=.5+n*.2-(cr<-.3?.3:0);
        uint8_t v=c8(b*150+30);
        t.pixels[y][x]={v,v,c8(v-8),255};
    }
}

static void tex_concrete_stains(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.15,y*.15,6,4), s=_fractal(x*.08,y*.08,66,3);
        uint8_t v=c8((.48+n*.18)*140+30), st=c8((.5+s*.3)*60);
        t.pixels[y][x]={c8(v-st/2),c8(v-st/3),c8(v-st),255};
    }
}

static void tex_vent_grate(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.5,y*.5,7,2)*.08;
        bool f=(y%8<3)||(x%8<3);
        uint8_t v=c8((f?.5+n:.05+n)*180+20);
        t.pixels[y][x]={v,c8(v+5),c8(v+10),255};
    }
}

static void tex_pipe_horiz(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.3,y*.3,8,2)*.1;
        int z=y%20;
        double b=(z<4?.2:z<8?.6:z<12?.75:z<16?.55:.2)+n;
        if (x%12==0||x%12==1) b-=.1;
        uint8_t v=c8(b*190+20);
        t.pixels[y][x]={v,c8(v+2),c8(v+5),255};
    }
}

static void tex_warn_yellow(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.4,y*.4,9,2)*.05;
        bool yw=((x+y)/8)%2==0;
        if (yw) t.pixels[y][x]={c8(220+n*20),c8(180+n*20),20,255};
        else    t.pixels[y][x]={20,20,c8(20+n*10),255};
    }
}

static void tex_warn_red(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.4,y*.4,10,2)*.05;
        bool rd=((x+y)/8)%2==0;
        if (rd) t.pixels[y][x]={c8(220+n*20),20,20,255};
        else    t.pixels[y][x]={240,240,240,255};
    }
}

static void tex_dark_hull(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.2,y*.2,11,4)*.15;
        bool h=(y%32==0||y%32==1), v=(x%32==0||x%32==1);
        double b=.12+n+(h||v?.08:0);
        uint8_t c=c8(b*120+10);
        t.pixels[y][x]={c,c8(c+2),c8(c+8),255};
    }
}

static void tex_damaged_wires(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.25,y*.25,12,3)*.15, cr=_fractal(x*.6,y*.6,112,2);
        double b=.25+n-(cr<-.2?.2:0);
        bool w1=(x%20>8&&x%20<10&&y%3!=2);
        bool w2=(x%20>14&&x%20<16&&y%4!=3);
        bool w3=(x%20>3&&x%20<5&&y%5!=4);
        if      (w1) t.pixels[y][x]={200,150,20,255};
        else if (w2) t.pixels[y][x]={200,30,30,255};
        else if (w3) t.pixels[y][x]={30,30,200,255};
        else { uint8_t v=c8(b*150+20); t.pixels[y][x]={v,c8(v-5),c8(v-10),255}; }
    }
}

static void tex_glass_porthole(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double cx=x-32, cy=y-32, d=sqrt(cx*cx+cy*cy);
        double n=_fractal(x*.3,y*.3,13,2)*.1;
        bool frame=d>26||x<4||x>59||y<4||y>59;
        if (frame) { uint8_t v=c8(.45*180+20+n*20); t.pixels[y][x]={v,c8(v+3),c8(v+8),255}; }
        else {
            double s=_fractal(x*1.5,y*1.5,777,1);
            if (s>.7) t.pixels[y][x]={240,240,255,255};
            else      t.pixels[y][x]={5,5,c8(20+d*.5),255};
        }
    }
}

static void tex_energy_blue(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.3,y*.3,14,3)*.1, e=_fractal(x*.6,y*.1,14,2);
        bool h=(y%8==0||y%8==1), v=(x%32==0||x%32==1);
        if      (h||v) t.pixels[y][x]={20,80,200,255};
        else if (e>.3) t.pixels[y][x]={10,c8(40+e*80),c8(150+e*100),255};
        else { uint8_t c=c8((.15+n)*100+15); t.pixels[y][x]={c,c8(c+10),c8(c+40),255}; }
    }
}

static void tex_energy_red(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.3,y*.3,15,3)*.1, e=_fractal(x*.6,y*.1,15,2);
        bool h=(y%8==0||y%8==1), v=(x%32==0||x%32==1);
        if      (h||v) t.pixels[y][x]={220,30,20,255};
        else if (e>.3) t.pixels[y][x]={c8(150+e*100),c8(20+e*30),10,255};
        else { uint8_t c=c8((.15+n)*100+15); t.pixels[y][x]={c8(c+30),c,c,255}; }
    }
}

static void tex_floor_tile(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.4,y*.4,16,3)*.08;
        bool h=(y%16==0||y%16==1), v=(x%16==0||x%16==1);
        double b=.38+n-(h||v?.12:0);
        uint8_t c=c8(b*170+25);
        t.pixels[y][x]={c,c8(c+2),c8(c+5),255};
    }
}

static void tex_floor_grate(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.5,y*.5,17,2)*.06;
        bool f=(y%6<2)||(x%6<2);
        if (!f) { t.pixels[y][x]={15,15,20,255}; continue; }
        uint8_t v=c8((.55+n)*180+15);
        t.pixels[y][x]={v,c8(v+3),c8(v+8),255};
    }
}

static void tex_floor_concrete(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.18,y*.18,18,5), d=_fractal(x*.5,y*.5,18,2)*.1;
        uint8_t v=c8((.42+n*.2+d)*140+35);
        t.pixels[y][x]={v,c8(v-2),c8(v-6),255};
    }
}

static void tex_floor_marking(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.4,y*.4,19,2)*.06;
        bool h=(y%16==0||y%16==1), v=(x%16==0||x%16==1);
        bool arr=(y>20&&y<44)&&(x==32||(std::abs(x-32)==(y-20)/3&&y<35));
        if (arr) { t.pixels[y][x]={220,180,20,255}; continue; }
        double b=.3+n-(h||v?.1:0);
        uint8_t c=c8(b*150+25);
        t.pixels[y][x]={c,c,c8(c-5),255};
    }
}

static void tex_ceil_lamps(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.3,y*.3,20,3)*.08;
        bool lamp=(x>12&&x<52&&y>24&&y<40);
        bool rim=(x==12||x==52||y==24||y==40)&&lamp;
        if (lamp&&!rim) { t.pixels[y][x]={240,220,180,255}; continue; }
        if (rim)        { t.pixels[y][x]={100,100,100,255}; continue; }
        uint8_t v=c8((.35+n)*160+20);
        t.pixels[y][x]={v,c8(v+2),c8(v+6),255};
    }
}

static void tex_ceil_vent(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        bool f=(y%10<3)||(x%10<3)||(x<3||x>60||y<3||y>60);
        if (!f) { t.pixels[y][x]={15,15,20,255}; continue; }
        double n=_fractal(x*.4,y*.4,21,2)*.07;
        uint8_t v=c8((.5+n)*170+20);
        t.pixels[y][x]={v,c8(v+3),c8(v+8),255};
    }
}

static void tex_ceil_emergency(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.3,y*.3,22,3)*.08;
        bool lamp=(x>20&&x<44&&y>20&&y<44);
        if (lamp) { t.pixels[y][x]={200,30,20,255}; continue; }
        uint8_t v=c8((.1+n)*80+10);
        t.pixels[y][x]={c8(v+10),v,v,255};
    }
}

static void tex_ceil_pipes(Texture& t) {
    for (int y=0; y<TEX_SIZE; y++)
    for (int x=0; x<TEX_SIZE; x++) {
        double n=_fractal(x*.3,y*.3,23,3)*.08;
        bool p=(x>8&&x<16)||(x>28&&x<36)||(x>48&&x<56);
        bool cable=(y>28&&y<31);
        if (p) { uint8_t v=c8((.5+n)*190+20); t.pixels[y][x]={v,c8(v+3),c8(v+8),255}; }
        else if (cable) { t.pixels[y][x]={30,30,30,255}; }
        else { uint8_t v=c8((.25+n)*150+15); t.pixels[y][x]={v,c8(v+2),c8(v+5),255}; }
    }
}

inline void textures_init() {
    tex_metal_clean    (g_textures[0]);
    tex_metal_rust     (g_textures[1]);
    tex_metal_rivets   (g_textures[2]);
    tex_concrete_clean (g_textures[3]);
    tex_concrete_cracks(g_textures[4]);
    tex_concrete_stains(g_textures[5]);
    tex_vent_grate     (g_textures[6]);
    tex_pipe_horiz     (g_textures[7]);
    tex_warn_yellow    (g_textures[8]);
    tex_warn_red       (g_textures[9]);
    tex_dark_hull      (g_textures[10]);
    tex_damaged_wires  (g_textures[11]);
    tex_glass_porthole (g_textures[12]);
    tex_energy_blue    (g_textures[13]);
    tex_energy_red     (g_textures[14]);
    tex_floor_tile     (g_textures[15]);
    tex_floor_grate    (g_textures[16]);
    tex_floor_concrete (g_textures[17]);
    tex_floor_marking  (g_textures[18]);
    tex_ceil_lamps     (g_textures[19]);
    tex_ceil_vent      (g_textures[20]);
    tex_ceil_emergency (g_textures[21]);
    tex_ceil_pipes     (g_textures[22]);
}

inline const Color& tex_sample(int idx, int x, int y) {
    x = ((x % TEX_SIZE) + TEX_SIZE) % TEX_SIZE;
    y = ((y % TEX_SIZE) + TEX_SIZE) % TEX_SIZE;
    return g_textures[idx].pixels[y][x];
}
