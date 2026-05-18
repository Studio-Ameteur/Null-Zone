#pragma once
#include <SDL.h>
#include "common.h"
#include "world.h"
#include "player.h"
#include "entity.h"
#include "textures.h"

static const int RENDER_W = 640;
static const int RENDER_H = 360;
static const int RENDER_HALF_H = RENDER_H / 2;

static SDL_Texture*  g_sdlTex     = nullptr;
static SDL_Texture*  g_texCache[NUM_TEXTURES] = {};
static uint32_t      g_pixels[RENDER_W * RENDER_H];
static double        g_zbuf[RENDER_W];
static SDL_Renderer* g_rend_ptr   = nullptr;

static const Color FOG_COLOR  = {10, 8, 15, 255};
static const double FOG_END   = 16.0;

inline void renderer_init(SDL_Renderer* rend) {
    g_rend_ptr = rend;
    g_sdlTex = SDL_CreateTexture(rend,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        RENDER_W, RENDER_H);

    for (int i=0; i<NUM_TEXTURES; i++) {
        g_texCache[i] = SDL_CreateTexture(rend,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STATIC,
            TEX_SIZE, TEX_SIZE);
        if (!g_texCache[i]) continue;
        uint32_t buf[TEX_SIZE*TEX_SIZE];
        for (int y=0;y<TEX_SIZE;y++)
        for (int x=0;x<TEX_SIZE;x++) {
            const Color& c = g_textures[i].pixels[y][x];
            buf[y*TEX_SIZE+x] = 0xFF000000u|((uint32_t)c.r<<16)|((uint32_t)c.g<<8)|c.b;
        }
        SDL_UpdateTexture(g_texCache[i], nullptr, buf, TEX_SIZE*4);
    }
}

inline void renderer_destroy() {
    for (int i=0;i<NUM_TEXTURES;i++)
        if (g_texCache[i]) { SDL_DestroyTexture(g_texCache[i]); g_texCache[i]=nullptr; }
    if (g_sdlTex) { SDL_DestroyTexture(g_sdlTex); g_sdlTex=nullptr; }
}

static inline void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if ((unsigned)x>=(unsigned)RENDER_W||(unsigned)y>=(unsigned)RENDER_H) return;
    g_pixels[y*RENDER_W+x] = 0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
}

static inline double fog_f(double dist) {
    double f = dist / FOG_END;
    return f>1.0?1.0:f<0.0?0.0:f;
}

static inline void apply_fog(uint8_t& r, uint8_t& g, uint8_t& b, double dist) {
    double f = fog_f(dist);
    double k = 1.0-f;
    r=(uint8_t)(r*k + FOG_COLOR.r*f);
    g=(uint8_t)(g*k + FOG_COLOR.g*f);
    b=(uint8_t)(b*k + FOG_COLOR.b*f);
}

static inline void apply_light(uint8_t& r, uint8_t& g, uint8_t& b, Vec2 wp) {
    double lr=0,lg=0,lb=0;
    for (int ri=0;ri<g_world.roomCount;ri++) {
        Room& rm=g_world.rooms[ri];
        if (rm.state==ROOM_UNLOADED) continue;
        for (int li=0;li<rm.lightCount;li++) {
            PointLight& pl=rm.lights[li];
            if (!pl.active) continue;
            double dx=wp.x-pl.pos.x, dy=wp.y-pl.pos.y;
            double d2=dx*dx+dy*dy;
            if (d2>pl.radius*pl.radius) continue;
            double att=(1.0-sqrt(d2)/pl.radius);
            att=att*att*pl.intensity;
            lr+=att*pl.color.r;
            lg+=att*pl.color.g;
            lb+=att*pl.color.b;
        }
    }
    r=(uint8_t)std::min(255.0,(double)r+lr*0.5);
    g=(uint8_t)std::min(255.0,(double)g+lg*0.5);
    b=(uint8_t)std::min(255.0,(double)b+lb*0.5);
}

static void render_floor_ceil(const Player& p) {
    double darkF = std::max(0.0, 1.0 - 1.0/FOG_END);

    uint8_t cr = (uint8_t)(FOG_COLOR.r + (18-FOG_COLOR.r)*darkF*0.3);
    uint8_t cg = (uint8_t)(FOG_COLOR.g + (14-FOG_COLOR.g)*darkF*0.3);
    uint8_t cb = (uint8_t)(FOG_COLOR.b + (22-FOG_COLOR.b)*darkF*0.3);

    uint8_t fr = (uint8_t)(FOG_COLOR.r + (25-FOG_COLOR.r)*darkF*0.4);
    uint8_t fg = (uint8_t)(FOG_COLOR.g + (20-FOG_COLOR.g)*darkF*0.4);
    uint8_t fb = (uint8_t)(FOG_COLOR.b + (30-FOG_COLOR.b)*darkF*0.4);

    uint32_t ceilPx  = 0xFF000000u|((uint32_t)cr<<16)|((uint32_t)cg<<8)|cb;
    uint32_t floorPx = 0xFF000000u|((uint32_t)fr<<16)|((uint32_t)fg<<8)|fb;

    for (int y=0;y<RENDER_HALF_H;y++)
    for (int x=0;x<RENDER_W;x++)
        g_pixels[y*RENDER_W+x] = ceilPx;

    for (int y=RENDER_HALF_H;y<RENDER_H;y++)
    for (int x=0;x<RENDER_W;x++)
        g_pixels[y*RENDER_W+x] = floorPx;
}

static void render_walls(const Player& p) {
    for (int x=0; x<RENDER_W; x++) {
        double camX = 2.0*x/RENDER_W - 1.0;
        double rdx  = p.dir.x + p.plane.x*camX;
        double rdy  = p.dir.y + p.plane.y*camX;

        int mapX=(int)p.pos.x, mapY=(int)p.pos.y;
        double ddx = fabs(rdx)<EPSILON?1e30:fabs(1.0/rdx);
        double ddy = fabs(rdy)<EPSILON?1e30:fabs(1.0/rdy);

        int stepX, stepY;
        double sdx, sdy;
        if(rdx<0){stepX=-1;sdx=(p.pos.x-mapX)*ddx;}
        else     {stepX= 1;sdx=(mapX+1.0-p.pos.x)*ddx;}
        if(rdy<0){stepY=-1;sdy=(p.pos.y-mapY)*ddy;}
        else     {stepY= 1;sdy=(mapY+1.0-p.pos.y)*ddy;}

        bool hit=false,side=false;
        int hitX=mapX,hitY=mapY;
        for(int step=0;step<MAP_MAX&&!hit;step++) {
            if(sdx<sdy){sdx+=ddx;mapX+=stepX;side=false;}
            else        {sdy+=ddy;mapY+=stepY;side=true;}
            if(!world_in_bounds(mapX,mapY)) break;
            TileType tt=g_world.map[mapY][mapX].type;
            if(tt==TILE_WALL||(tt==TILE_DOOR&&!g_world.map[mapY][mapX].doorOpen)){
                hit=true;hitX=mapX;hitY=mapY;
            }
        }

        if(!hit){g_zbuf[x]=1e30;continue;}

        double wallDist=side
            ?(mapY-p.pos.y+(1-stepY)*0.5)/rdy
            :(mapX-p.pos.x+(1-stepX)*0.5)/rdx;
        if(wallDist<=0.001){g_zbuf[x]=1e30;continue;}
        g_zbuf[x]=wallDist;

        int lineH=(int)(RENDER_H/wallDist);
        int drawS=std::max(0,RENDER_HALF_H-lineH/2);
        int drawE=std::min(RENDER_H-1,RENDER_HALF_H+lineH/2);

        double wallX=side?p.pos.x+wallDist*rdx:p.pos.y+wallDist*rdy;
        wallX-=floor(wallX);
        int texIdx=g_world.map[hitY][hitX].texWall;
        int texX=(int)(wallX*TEX_SIZE);
        if((!side&&rdx>0)||(side&&rdy<0)) texX=TEX_SIZE-1-texX;
        texX=std::max(0,std::min(TEX_SIZE-1,texX));

        Vec2 wp;
        if(!side){wp.x=(double)hitX+(rdx>0?0:1);wp.y=p.pos.y+wallDist*rdy;}
        else     {wp.x=p.pos.x+wallDist*rdx;wp.y=(double)hitY+(rdy>0?0:1);}

        double step_tex=(double)TEX_SIZE/std::max(1,lineH);
        double tex_pos=(drawS-RENDER_HALF_H+lineH*0.5)*step_tex;
        double distF=std::max(0.05,1.0-wallDist/FOG_END);
        double sideF=side?0.75:1.0;

        for(int y=drawS;y<=drawE;y++,tex_pos+=step_tex) {
            int texY=std::max(0,std::min(TEX_SIZE-1,(int)tex_pos));
            const Color& c=tex_sample(texIdx,texX,texY);
            uint8_t r=(uint8_t)(c.r*distF*sideF);
            uint8_t g=(uint8_t)(c.g*distF*sideF);
            uint8_t b=(uint8_t)(c.b*distF*sideF);
            apply_light(r,g,b,wp);
            apply_fog(r,g,b,wallDist);
            put_pixel(x,y,r,g,b);
        }
    }
}

static Color enemy_pixel_fast(EnemyType et, int x, int y, int frame) {
    static const Color bc[15]={
        {80,160,220},{200,60,40},{160,80,40},{80,40,160},{200,200,60},
        {120,200,80},{60,120,200},{220,120,40},{200,80,20},{160,160,200},
        {80,200,160},{160,160,160},{200,40,40},{160,120,40},{200,150,20}
    };
    double cx=x-32.0,cy=y-38.0;
    double hx=x-32.0,hy=y-20.0;
    bool body=(cx*cx/18.0/18.0+cy*cy/22.0/22.0)<1.0;
    bool head=(hx*hx+hy*hy)<144.0;
    bool eye1=((x-26.0)*(x-26.0)+(y-18.0)*(y-18.0))<9.0;
    bool eye2=((x-38.0)*(x-38.0)+(y-18.0)*(y-18.0))<9.0;
    if(!body&&!head) return {0,0,0,0};
    if(eye1||eye2)   return {240,60,20,255};
    if(head) return bc[et]*0.75;
    double bob=sin(frame*0.8)*1.5;
    return y>38+bob?bc[et]:bc[et]*0.65;
}

static Color item_pixel_fast(ItemType it, int x, int y) {
    double dx=x-32.0,dy=y-32.0,d=sqrt(dx*dx+dy*dy);
    switch(it){
    case ITEM_HEALTH_SMALL:
        if((abs(x-32)<4&&abs(y-32)<12)||(abs(x-32)<12&&abs(y-32)<4)) return {220,40,40,255};
        return {0,0,0,0};
    case ITEM_HEALTH_BIG:
        if(d<18&&((abs(x-32)<6&&abs(y-32)<16)||(abs(x-32)<16&&abs(y-32)<6))) return {220,40,40,255};
        return {0,0,0,0};
    case ITEM_ARMOR:
        if(d<18&&y>20) return {60,120,220,255};
        return {0,0,0,0};
    case ITEM_WEAPON:
        if(d<16&&y>24) return {180,150,80,255};
        return {0,0,0,0};
    default:
        if(d<10) return {200,200,60,255};
        return {0,0,0,0};
    }
}

struct SprOrder{int idx;double dist;};
static SprOrder g_spr[MAX_ENTITIES];
static int      g_sprN=0;

static void render_sprites(const Player& p) {
    g_sprN=0;
    for(int i=0;i<g_entityCount;i++){
        Entity& e=g_entities[i];
        if(!e.active||!e.inFOV) continue;
        if(e.type==ENT_ENEMY&&e.state==ES_DEAD) continue;
        if(e.invisible&&e.pos.distTo(p.pos)>3.0) continue;
        g_spr[g_sprN++]={i,e.dist};
    }
    for(int i=0;i<g_sprN-1;i++)
    for(int j=i+1;j<g_sprN;j++)
        if(g_spr[j].dist>g_spr[i].dist) std::swap(g_spr[i],g_spr[j]);

    for(int si=0;si<g_sprN;si++){
        Entity& e=g_entities[g_spr[si].idx];
        double sx=e.pos.x-p.pos.x, sy=e.pos.y-p.pos.y;
        double invDet=1.0/(p.plane.x*p.dir.y-p.dir.x*p.plane.y+EPSILON);
        double transX=invDet*(p.dir.y*sx-p.dir.x*sy);
        double transY=invDet*(-p.plane.y*sx+p.plane.x*sy);
        if(transY<=0.1) continue;

        int sprH=abs((int)(RENDER_H/transY));
        int sprW=sprH;
        int scx=(int)(RENDER_W/2*(1.0+transX/transY));
        int dSX=std::max(0,scx-sprW/2);
        int dEX=std::min(RENDER_W-1,scx+sprW/2);
        int dSY=std::max(0,RENDER_HALF_H-sprH/2);
        int dEY=std::min(RENDER_H-1,RENDER_HALF_H+sprH/2);
        if(dSX>=dEX||dSY>=dEY) continue;

        int lodStep=(e.dist>10.0)?2:1;
        double distF=std::max(0.05,1.0-transY/FOG_END);

        for(int x=dSX;x<=dEX;x+=lodStep){
            if(transY>=g_zbuf[x]) continue;
            int texX=(x-(scx-sprW/2))*TEX_SIZE/std::max(1,sprW);
            texX=std::max(0,std::min(TEX_SIZE-1,texX));
            for(int y=dSY;y<=dEY;y+=lodStep){
                int texY=(y-dSY)*TEX_SIZE/std::max(1,dEY-dSY);
                texY=std::max(0,std::min(TEX_SIZE-1,texY));
                Color c={0,0,0,0};
                if(e.type==ENT_ENEMY)
                    c=enemy_pixel_fast(e.enemyType,texX,texY,e.animFrame);
                else if(e.type==ENT_ITEM)
                    c=item_pixel_fast(e.itemType,texX,texY);
                if(c.a==0) continue;
                uint8_t r=(uint8_t)(c.r*distF);
                uint8_t g=(uint8_t)(c.g*distF);
                uint8_t b=(uint8_t)(c.b*distF);
                apply_fog(r,g,b,transY);
                put_pixel(x,y,r,g,b);
                if(lodStep==2){
                    put_pixel(x+1,y,r,g,b);
                    put_pixel(x,y+1,r,g,b);
                    put_pixel(x+1,y+1,r,g,b);
                }
            }
        }
    }
}

static void render_weapon_sprite(const Player& p) {
    int wi=p.currentWeapon;
    static const Color wpnColors[MAX_WEAPONS]={
        {180,140,100},{200,180,150},{180,150,120},{160,160,170},
        {200,190,180},{220,100,80},{80,160,220},{180,150,120},
        {220,120,60},{180,220,220},{200,180,100},{180,180,190},
        {100,180,220},{200,160,80},{80,200,80}
    };
    Color wc=wpnColors[wi];
    int bob=(int)(p.bobAmt*20);
    int bobs=(int)(p.bobSide*15);

    int wx=RENDER_W/2-50+bobs;
    int wy=RENDER_H-110+bob;
    int ww=100, wh=100;

    for(int y=wy;y<wy+wh&&y<RENDER_H;y++)
    for(int x=wx;x<wx+ww&&x<RENDER_W;x++) {
        if(x<0||x>=RENDER_W||y<0||y>=RENDER_H) continue;
        int lx=x-wx, ly=y-wy;
        bool draw=false;
        uint8_t r=wc.r,g=wc.g,b=wc.b;

        switch(wi) {
        case 0:
            draw=(ly>30&&lx>20&&lx<80)||(ly>50&&lx>30&&lx<70);
            if(lx>45&&lx<55&&ly>10&&ly<35){draw=true;r=200;g=160;b=120;}
            break;
        case 1:
            draw=(ly>40&&lx>30&&lx<80&&ly<80)||(lx>60&&lx<80&&ly>20&&ly<50);
            if(lx>60&&lx<75&&ly>20&&ly<35){r=(uint8_t)(r*0.7);g=(uint8_t)(g*0.7);b=(uint8_t)(b*0.7);}
            break;
        case 2:case 7:
            draw=(ly>50&&lx>10&&lx<90&&ly<85)||(lx>40&&lx<60&&ly>20&&ly<55);
            break;
        case 3:
            draw=(ly>45&&lx>25&&lx<85&&ly<80)||(lx>55&&lx<75&&ly>15&&ly<50);
            break;
        case 4:
            draw=(ly>50&&lx>35&&lx<75&&ly<85)||(lx>55&&lx<65&&ly>10&&ly<55);
            break;
        case 5:
            draw=(ly>40&&lx>20&&lx<80&&ly<85)||(lx>30&&lx<50&&ly>15&&ly<45);
            if(lx>30&&lx<50&&ly>15&&ly<30){r=220;g=100;b=80;}
            break;
        case 6:
            draw=(ly>45&&lx>25&&lx<80&&ly<80)||(lx>50&&lx<70&&ly>15&&ly<50);
            if(lx>50&&lx<70&&ly>15&&ly<35){r=80;g=160;b=220;}
            break;
        case 8:
            draw=(ly>45&&lx>15&&lx<85&&ly<80)||(lx>55&&lx<75&&ly>20&&ly<50);
            if(lx>55&&lx<75&&ly>20&&ly<40){r=220;g=120;b=60;}
            break;
        case 9:
            draw=(ly>50&&lx>30&&lx<80&&ly<85)||(lx>58&&lx<68&&ly>10&&ly<55);
            if(lx>58&&lx<68&&ly>10&&ly<30){r=180;g=220;b=220;}
            break;
        case 11:
            draw=(ly>45&&lx>5&&lx<95&&ly<80)||(lx>40&&lx<60&&ly>15&&ly<50);
            break;
        case 12:
            draw=(ly>45&&lx>25&&lx<80&&ly<80)||(lx>50&&lx<70&&ly>15&&ly<50);
            if(lx>50&&lx<70&&ly>15&&ly<35){r=100;g=180;b=220;}
            break;
        case 13:
            draw=(ly>40&&lx>15&&lx<85&&ly<85);
            if(ly>40&&lx>40&&lx<60){r=200;g=160;b=80;}
            break;
        case 14:
            draw=(ly>35&&lx>10&&lx<90&&ly<85)||(lx>40&&lx<60&&ly>10&&ly<40);
            if(lx>40&&lx<60&&ly>10&&ly<40){r=80;g=200;b=80;}
            break;
        default:
            draw=(ly>50&&lx>25&&lx<80&&ly<85);
            break;
        }

        if(draw) put_pixel(x,y,r,g,b);
    }
}

static void render_crosshair() {
    int cx=RENDER_W/2, cy=RENDER_H/2;
    for(int i=3;i<=6;i++){
        put_pixel(cx+i,cy,200,200,200);
        put_pixel(cx-i,cy,200,200,200);
        put_pixel(cx,cy+i,200,200,200);
        put_pixel(cx,cy-i,200,200,200);
    }
    put_pixel(cx,cy,255,80,80);
    put_pixel(cx+1,cy,255,80,80);
    put_pixel(cx-1,cy,255,80,80);
    put_pixel(cx,cy+1,255,80,80);
    put_pixel(cx,cy-1,255,80,80);
}

static void render_damage_flash(double flash) {
    if(flash<=0) return;
    uint8_t a=(uint8_t)(flash*80);
    for(int i=0;i<RENDER_W*RENDER_H;i++){
        uint32_t& px=g_pixels[i];
        uint8_t r=std::min(255,(int)((px>>16)&0xFF)+a);
        px=(px&0xFF00FFFFu)|((uint32_t)r<<16);
    }
}

static void render_pickup_flash(double flash) {
    if(flash<=0) return;
    uint8_t a=(uint8_t)(flash*35);
    for(int i=0;i<RENDER_W*RENDER_H;i++){
        uint32_t& px=g_pixels[i];
        uint8_t g=std::min(255,(int)((px>>8)&0xFF)+a);
        px=(px&0xFFFF00FFu)|((uint32_t)g<<8);
    }
}

inline void renderer_draw_frame(SDL_Renderer* rend, const Player& p) {
    render_floor_ceil(p);
    render_walls(p);
    render_sprites(p);
    render_weapon_sprite(p);
    render_crosshair();
    if(p.damageFlash>0) render_damage_flash(p.damageFlash);
    if(p.pickupFlash>0) render_pickup_flash(p.pickupFlash);

    SDL_UpdateTexture(g_sdlTex,nullptr,g_pixels,RENDER_W*4);
    SDL_Rect dst={0,0,SCREEN_W,SCREEN_H};
    SDL_RenderCopy(rend,g_sdlTex,nullptr,&dst);
}
