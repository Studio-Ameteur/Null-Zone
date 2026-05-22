#pragma once
#include <SDL.h>
#include "common.h"
#include "world.h"
#include "player.h"
#include "entity.h"
#include "textures.h"
#include "sprites.h"

static const int RENDER_W      = 640;
static const int RENDER_H      = 360;
static const int RENDER_HALF_H = RENDER_H / 2;

static SDL_Texture* g_sdlTex   = nullptr;
static uint32_t     g_pixels[RENDER_W * RENDER_H];
static double       g_zbuf[RENDER_W];

static const Color FOG_COLOR = {10, 8, 15, 255};
static const double FOG_END  = 16.0;

inline void renderer_init(SDL_Renderer* rend) {
    g_sdlTex = SDL_CreateTexture(rend,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        RENDER_W, RENDER_H);
    sprites_init(rend);
}

inline void renderer_destroy() {
    sprites_shutdown();
    if(g_sdlTex){SDL_DestroyTexture(g_sdlTex);g_sdlTex=nullptr;}
}

static inline void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if((unsigned)x>=(unsigned)RENDER_W||(unsigned)y>=(unsigned)RENDER_H) return;
    g_pixels[y*RENDER_W+x]=0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
}

static inline void apply_fog(uint8_t& r,uint8_t& g,uint8_t& b,double dist) {
    double f=dist/FOG_END; if(f>1)f=1; if(f<0)f=0;
    double k=1-f;
    r=(uint8_t)(r*k+FOG_COLOR.r*f);
    g=(uint8_t)(g*k+FOG_COLOR.g*f);
    b=(uint8_t)(b*k+FOG_COLOR.b*f);
}

static inline void apply_light(uint8_t& r,uint8_t& g,uint8_t& b,Vec2 wp) {
    double lr=0,lg=0,lb=0;
    for(int ri=0;ri<g_world.roomCount;ri++){
        Room& rm=g_world.rooms[ri];
        if(rm.state==ROOM_UNLOADED) continue;
        for(int li=0;li<rm.lightCount;li++){
            PointLight& pl=rm.lights[li];
            if(!pl.active) continue;
            double dx=wp.x-pl.pos.x,dy=wp.y-pl.pos.y;
            double d2=dx*dx+dy*dy;
            if(d2>pl.radius*pl.radius) continue;
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

static void render_floor_ceil() {
    uint8_t cr=18,cg=14,cb=24;
    uint8_t fr=22,fg=18,fb=28;
    uint32_t ceilPx =0xFF000000u|((uint32_t)cr<<16)|((uint32_t)cg<<8)|cb;
    uint32_t floorPx=0xFF000000u|((uint32_t)fr<<16)|((uint32_t)fg<<8)|fb;
    for(int y=0;y<RENDER_HALF_H;y++)
    for(int x=0;x<RENDER_W;x++)
        g_pixels[y*RENDER_W+x]=ceilPx;
    for(int y=RENDER_HALF_H;y<RENDER_H;y++)
    for(int x=0;x<RENDER_W;x++)
        g_pixels[y*RENDER_W+x]=floorPx;
}

static void render_walls(const Player& p) {
    for(int x=0;x<RENDER_W;x++){
        double camX=2.0*x/RENDER_W-1.0;
        double rdx=p.dir.x+p.plane.x*camX;
        double rdy=p.dir.y+p.plane.y*camX;

        int mapX=(int)p.pos.x,mapY=(int)p.pos.y;
        double ddx=fabs(rdx)<EPSILON?1e30:fabs(1.0/rdx);
        double ddy=fabs(rdy)<EPSILON?1e30:fabs(1.0/rdy);

        int stepX,stepY;
        double sdx,sdy;
        if(rdx<0){stepX=-1;sdx=(p.pos.x-mapX)*ddx;}
        else     {stepX= 1;sdx=(mapX+1.0-p.pos.x)*ddx;}
        if(rdy<0){stepY=-1;sdy=(p.pos.y-mapY)*ddy;}
        else     {stepY= 1;sdy=(mapY+1.0-p.pos.y)*ddy;}

        bool hit=false,side=false;
        int hitX=mapX,hitY=mapY;
        for(int step=0;step<MAP_MAX&&!hit;step++){
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
        double sideF=side?0.72:1.0;

        for(int y=drawS;y<=drawE;y++,tex_pos+=step_tex){
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

static void render_crosshair() {
    int cx=RENDER_W/2,cy=RENDER_H/2;
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

struct SprOrd{int idx;double dist;};
static SprOrd g_spr[MAX_ENTITIES];
static int    g_sprN=0;

static void render_sprites_software(const Player& p) {
    for(int si=0;si<g_sprN;si++){
        Entity& e=g_entities[g_spr[si].idx];
        double sx=e.pos.x-p.pos.x,sy=e.pos.y-p.pos.y;
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

        double distF=std::max(0.05,1.0-transY/FOG_END);

        if(e.type==ENT_ITEM){
            for(int x=dSX;x<=dEX;x++){
                if(transY>=g_zbuf[x]) continue;
                int texX=(x-(scx-sprW/2))*TEX_SIZE/std::max(1,sprW);
                texX=std::max(0,std::min(TEX_SIZE-1,texX));
                for(int y=dSY;y<=dEY;y++){
                    int texY=(y-dSY)*TEX_SIZE/std::max(1,dEY-dSY);
                    texY=std::max(0,std::min(TEX_SIZE-1,texY));
                    double dx=texX-32.0,dy=texY-32.0,d=sqrt(dx*dx+dy*dy);
                    if(d>14) continue;
                    uint8_t r=200,g=200,b=60;
                    switch(e.itemType){
                    case ITEM_HEALTH_SMALL:case ITEM_HEALTH_BIG: r=220;g=40;b=40; break;
                    case ITEM_ARMOR: r=60;g=120;b=220; break;
                    case ITEM_WEAPON: r=180;g=150;b=80; break;
                    default: r=200;g=200;b=60; break;
                    }
                    uint8_t rf=(uint8_t)(r*distF);
                    uint8_t gf=(uint8_t)(g*distF);
                    uint8_t bf=(uint8_t)(b*distF);
                    apply_fog(rf,gf,bf,transY);
                    put_pixel(x,y,rf,gf,bf);
                }
            }
        }
    }
}

inline void renderer_draw_frame(SDL_Renderer* rend, const Player& p) {
    render_floor_ceil();
    render_walls(p);

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

    render_sprites_software(p);

    if(p.damageFlash>0) render_damage_flash(p.damageFlash);
    if(p.pickupFlash>0) render_pickup_flash(p.pickupFlash);
    render_crosshair();

    SDL_UpdateTexture(g_sdlTex,nullptr,g_pixels,RENDER_W*4);
    SDL_Rect dst={0,0,0,0};
    SDL_GetRendererOutputSize(rend,&dst.w,&dst.h);
    SDL_RenderCopy(rend,g_sdlTex,nullptr,&dst);

    for(int si=0;si<g_sprN;si++){
        Entity& e=g_entities[g_spr[si].idx];
        if(e.type!=ENT_ENEMY) continue;

        double sx=e.pos.x-p.pos.x,sy=e.pos.y-p.pos.y;
        double invDet=1.0/(p.plane.x*p.dir.y-p.dir.x*p.plane.y+EPSILON);
        double transX=invDet*(p.dir.y*sx-p.dir.x*sy);
        double transY=invDet*(-p.plane.y*sx+p.plane.x*sy);
        if(transY<=0.1) continue;

        int sw=dst.w,sh=dst.h;
        int sprH=abs((int)(sh/transY));
        int scx=(int)(sw/2*(1.0+transX/transY));
        int scy=sh/2;

        if(scx<-sprH||scx>sw+sprH) continue;

        double distF=std::max(0.05,1.0-transY/FOG_END);
        int state=enemy_anim_state(e.state);

        sprites_draw_enemy(rend,
            (int)e.enemyType,
            e.animFrame,
            state,
            scx, scy,
            sprH,
            distF);
    }

    int sw=dst.w,sh=dst.h;
    if(!sprites_draw_weapon(rend,p.currentWeapon,p.weaponAnimFrame,p.bobAmt,p.bobSide,sw,sh)){
        int wi=p.currentWeapon;
        static const Color wc[MAX_WEAPONS]={
            {180,140,100},{200,180,150},{180,150,120},{160,160,170},
            {200,190,180},{220,100,80},{80,160,220},{180,150,120},
            {220,120,60},{180,220,220},{200,180,100},{180,180,190},
            {100,180,220},{200,160,80},{80,200,80}
        };
        int bob=(int)(p.bobAmt*20);
        int bobs=(int)(p.bobSide*15);
        int wx=sw/2-sw/16+bobs;
        int wy=sh-sh/4+bob;
        int ww=sw/8,wh=sh/4;
        SDL_SetRenderDrawColor(rend,wc[wi].r,wc[wi].g,wc[wi].b,255);
        SDL_Rect wr={wx,wy,ww,wh};
        SDL_RenderFillRect(rend,&wr);
        SDL_SetRenderDrawColor(rend,
            (uint8_t)(wc[wi].r*0.7),
            (uint8_t)(wc[wi].g*0.7),
            (uint8_t)(wc[wi].b*0.7),255);
        SDL_RenderDrawRect(rend,&wr);
    }
}
