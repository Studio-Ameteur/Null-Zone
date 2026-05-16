#pragma once
#include <SDL.h>
#include "common.h"
#include "world.h"
#include "player.h"
#include "entity.h"
#include "textures.h"

static uint32_t    g_pixels[SCREEN_W * SCREEN_H];
static double      g_zbuf[SCREEN_W];
static SDL_Texture* g_sdlTex = nullptr;

static const Color FOG_COLOR = {10, 8, 15, 255};
static const double FOG_START = 5.0;
static const double FOG_END   = 20.0;

inline void renderer_init(SDL_Renderer* rend) {
    g_sdlTex = SDL_CreateTexture(rend,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_W, SCREEN_H);
}

inline void renderer_destroy() {
    if (g_sdlTex) { SDL_DestroyTexture(g_sdlTex); g_sdlTex=nullptr; }
}

static inline void put_pixel(int x, int y, const Color& c) {
    if ((unsigned)x>=(unsigned)SCREEN_W||(unsigned)y>=(unsigned)SCREEN_H) return;
    g_pixels[y*SCREEN_W+x] = 0xFF000000u
        | ((uint32_t)c.r<<16)
        | ((uint32_t)c.g<<8)
        |  (uint32_t)c.b;
}

static inline void fill_rect(int x, int y, int w, int h, const Color& c) {
    for (int ry=y; ry<y+h; ry++)
    for (int rx=x; rx<x+w; rx++)
        put_pixel(rx, ry, c);
}

static inline double fog_factor(double dist) {
    double f = (dist - FOG_START) / (FOG_END - FOG_START);
    return std::max(0.0, std::min(1.0, f));
}

static Color apply_lighting(Color base, Vec2 worldPos, double dist, bool side) {
    if (side) { base.r=base.r*3/4; base.g=base.g*3/4; base.b=base.b*3/4; }

    double lr=0, lg=0, lb=0;
    for (int ri=0; ri<g_world.roomCount; ri++) {
        Room& rm = g_world.rooms[ri];
        if (rm.state == ROOM_UNLOADED) continue;
        for (int li=0; li<rm.lightCount; li++) {
            PointLight& pl = rm.lights[li];
            if (!pl.active) continue;
            double dx = worldPos.x - pl.pos.x;
            double dy = worldPos.y - pl.pos.y;
            double d  = sqrt(dx*dx+dy*dy);
            if (d > pl.radius) continue;
            double att = 1.0 - d/pl.radius;
            att = att*att * pl.intensity;
            lr += att * pl.color.r;
            lg += att * pl.color.g;
            lb += att * pl.color.b;
        }
    }

    double distF = std::max(0.05, 1.0 - dist/FOG_END);
    double r = base.r*distF + lr*0.6;
    double g = base.g*distF + lg*0.6;
    double b = base.b*distF + lb*0.6;

    Color lit = {
        (uint8_t)std::min(255.0, r),
        (uint8_t)std::min(255.0, g),
        (uint8_t)std::min(255.0, b),
        255
    };
    double ff = fog_factor(dist);
    return lit.blend(FOG_COLOR, ff);
}

static void render_floor_ceil(const Player& p) {
    for (int y=HALF_H+1; y<SCREEN_H; y++) {
        double rowDist = (double)HALF_H / (y - HALF_H + EPSILON);
        double fstepX  = rowDist * 2.0 * p.plane.x / SCREEN_W;
        double fstepY  = rowDist * 2.0 * p.plane.y / SCREEN_W;
        double floorX  = p.pos.x + rowDist*(p.dir.x - p.plane.x);
        double floorY  = p.pos.y + rowDist*(p.dir.y - p.plane.y);

        double ff  = fog_factor(rowDist);
        double distF = std::max(0.05, 1.0 - rowDist/FOG_END);

        for (int x=0; x<SCREEN_W; x++, floorX+=fstepX, floorY+=fstepY) {
            int mx = (int)floorX, my = (int)floorY;
            int tx = (int)(floorX*TEX_SIZE) & (TEX_SIZE-1);
            int ty = (int)(floorY*TEX_SIZE) & (TEX_SIZE-1);

            int floorTex = world_in_bounds(mx,my) ? g_world.map[my][mx].texFloor : 17;
            int ceilTex  = world_in_bounds(mx,my) ? g_world.map[my][mx].texCeil  : 22;

            Color fc = tex_sample(floorTex, tx, ty) * distF;
            Color cc = tex_sample(ceilTex,  tx, ty) * distF;
            fc = fc.blend(FOG_COLOR, ff);
            cc = cc.blend(FOG_COLOR, ff);

            put_pixel(x, y,              fc);
            put_pixel(x, SCREEN_H-1-y,  cc);
        }
    }
}

static void render_walls(const Player& p) {
    for (int x=0; x<NUM_RAYS; x++) {
        double camX = 2.0*x/NUM_RAYS - 1.0;
        double rdx  = p.dir.x + p.plane.x*camX;
        double rdy  = p.dir.y + p.plane.y*camX;

        int mapX=(int)p.pos.x, mapY=(int)p.pos.y;
        double ddx = fabs(rdx)<EPSILON ? 1e30 : fabs(1.0/rdx);
        double ddy = fabs(rdy)<EPSILON ? 1e30 : fabs(1.0/rdy);

        int stepX, stepY;
        double sdx, sdy;
        if (rdx<0){stepX=-1; sdx=(p.pos.x-mapX)*ddx;}
        else      {stepX= 1; sdx=(mapX+1.0-p.pos.x)*ddx;}
        if (rdy<0){stepY=-1; sdy=(p.pos.y-mapY)*ddy;}
        else      {stepY= 1; sdy=(mapY+1.0-p.pos.y)*ddy;}

        bool hit=false, side=false;
        int  hitX=mapX, hitY=mapY;

        for (int step=0; step<MAP_MAX && !hit; step++) {
            if (sdx < sdy){sdx+=ddx; mapX+=stepX; side=false;}
            else           {sdy+=ddy; mapY+=stepY; side=true; }
            if (!world_in_bounds(mapX,mapY)) break;
            TileType tt = g_world.map[mapY][mapX].type;
            if (tt==TILE_WALL||(tt==TILE_DOOR&&!g_world.map[mapY][mapX].doorOpen)) {
                hit=true; hitX=mapX; hitY=mapY;
            }
        }

        if (!hit) { g_zbuf[x]=1e30; continue; }

        double wallDist = side
            ? (mapY - p.pos.y + (1-stepY)*0.5) / rdy
            : (mapX - p.pos.x + (1-stepX)*0.5) / rdx;
        if (wallDist <= 0.001) { g_zbuf[x]=1e30; continue; }
        g_zbuf[x] = wallDist;

        int lineH = (int)(SCREEN_H / wallDist);
        int drawS = std::max(0, HALF_H - lineH/2);
        int drawE = std::min(SCREEN_H-1, HALF_H + lineH/2);

        double wallX = side
            ? p.pos.x + wallDist*rdx
            : p.pos.y + wallDist*rdy;
        wallX -= floor(wallX);

        int texIdx = g_world.map[hitY][hitX].texWall;
        int texX   = (int)(wallX * TEX_SIZE);
        if ((!side&&rdx>0)||(side&&rdy<0)) texX=TEX_SIZE-1-texX;
        texX = std::max(0, std::min(TEX_SIZE-1, texX));

        Vec2 worldHit;
        if (!side){worldHit.x=(double)hitX+(rdx>0?0:1);worldHit.y=p.pos.y+wallDist*rdy;}
        else      {worldHit.x=p.pos.x+wallDist*rdx;worldHit.y=(double)hitY+(rdy>0?0:1);}

        double step_tex = (double)TEX_SIZE / lineH;
        double tex_pos  = (drawS - HALF_H + lineH*0.5) * step_tex;

        for (int y=drawS; y<=drawE; y++, tex_pos+=step_tex) {
            int texY = std::max(0, std::min(TEX_SIZE-1, (int)tex_pos));
            Color c  = tex_sample(texIdx, texX, texY);
            c        = apply_lighting(c, worldHit, wallDist, side);
            put_pixel(x, y, c);
        }
    }
}

struct SpriteOrder { int idx; double dist; };
static SpriteOrder g_sprOrder[MAX_ENTITIES];
static int         g_sprCount = 0;

static Color enemy_sprite_pixel(EnemyType et, int x, int y, int frame) {
    static const Color bodyCol[15] = {
        {80,160,220},{200,60,40},{160,80,40},{80,40,160},{200,200,60},
        {120,200,80},{60,120,200},{220,120,40},{200,80,20},{160,160,200},
        {80,200,160},{160,160,160},{200,40,40},{160,120,40},{200,150,20}
    };
    double cx=32,cy=38;
    double bx=x-cx, by=y-cy;
    double headX=x-32.0, headY=y-20.0;
    bool body = (bx*bx/18.0/18.0 + by*by/22.0/22.0) < 1.0;
    bool head = (headX*headX+headY*headY) < 144.0;
    bool eye1 = ((x-26.0)*(x-26.0)+(y-18.0)*(y-18.0)) < 9.0;
    bool eye2 = ((x-38.0)*(x-38.0)+(y-18.0)*(y-18.0)) < 9.0;
    bool border = (x<2||x>61||y<2||y>61);
    if (border || (!body && !head)) return {0,0,0,0};
    if (eye1||eye2) return {240,60,20,255};
    Color bc = bodyCol[et];
    double bob = sin(frame*0.8)*1.5;
    if (head) return bc*0.75;
    return (y > cy+bob) ? bc : bc*0.65;
}

static Color item_sprite_pixel(ItemType it, int x, int y) {
    double cx=32, cy=32;
    double dx=x-cx, dy=y-cy;
    double d = sqrt(dx*dx+dy*dy);
    switch (it) {
    case ITEM_HEALTH_SMALL:
        if ((abs(x-32)<4&&abs(y-32)<12)||(abs(x-32)<12&&abs(y-32)<4))
            return {220,40,40,255};
        return {0,0,0,0};
    case ITEM_HEALTH_BIG:
        if (d<18&&((abs(x-32)<6&&abs(y-32)<16)||(abs(x-32)<16&&abs(y-32)<6)))
            return {220,40,40,255};
        return {0,0,0,0};
    case ITEM_ARMOR:
        if (d<18&&y>20&&(d<14||y<28))
            return {60,120,220,255};
        return {0,0,0,0};
    case ITEM_WEAPON:
        if (d<16&&y>24)
            return {180,150,80,255};
        return {0,0,0,0};
    default:
        if (d<10) return {200,200,60,255};
        return {0,0,0,0};
    }
}

static void render_sprites(const Player& p) {
    g_sprCount = 0;
    for (int i=0; i<g_entityCount; i++) {
        Entity& e = g_entities[i];
        if (!e.active || !e.inFOV) continue;
        if (e.type==ENT_ENEMY && e.state==ES_DEAD) continue;
        if (e.invisible && e.pos.distTo(p.pos)>3.0) continue;
        g_sprOrder[g_sprCount++] = {i, e.dist};
    }
    for (int i=0;i<g_sprCount-1;i++)
    for (int j=i+1;j<g_sprCount;j++)
        if (g_sprOrder[j].dist>g_sprOrder[i].dist)
            std::swap(g_sprOrder[i],g_sprOrder[j]);

    for (int si=0; si<g_sprCount; si++) {
        Entity& e = g_entities[g_sprOrder[si].idx];

        double sx = e.pos.x - p.pos.x;
        double sy = e.pos.y - p.pos.y;
        double invDet = 1.0/(p.plane.x*p.dir.y - p.dir.x*p.plane.y + EPSILON);
        double transX = invDet*(p.dir.y*sx  - p.dir.x*sy);
        double transY = invDet*(-p.plane.y*sx + p.plane.x*sy);
        if (transY <= 0.1) continue;

        int sprH  = abs((int)(SCREEN_H/transY));
        int sprW  = sprH;

        int screenX = (int)(SCREEN_W/2*(1.0 + transX/transY));
        int drawSX  = std::max(0, screenX - sprW/2);
        int drawEX  = std::min(SCREEN_W-1, screenX + sprW/2);
        int drawSY  = std::max(0, HALF_H - sprH/2);
        int drawEY  = std::min(SCREEN_H-1, HALF_H + sprH/2);
        if (drawSX>=drawEX||drawSY>=drawEY) continue;

        int lodStep = (e.dist>12.0) ? 2 : 1;

        for (int x=drawSX; x<=drawEX; x+=lodStep) {
            if (transY >= g_zbuf[x]) continue;
            int texX = (x - (screenX - sprW/2)) * TEX_SIZE / std::max(1,sprW);
            texX = std::max(0, std::min(TEX_SIZE-1, texX));

            for (int y=drawSY; y<=drawEY; y+=lodStep) {
                int texY = (y-drawSY)*TEX_SIZE / std::max(1,drawEY-drawSY);
                texY = std::max(0, std::min(TEX_SIZE-1, texY));

                Color c = {0,0,0,0};
                if (e.type==ENT_ENEMY)
                    c = enemy_sprite_pixel(e.enemyType, texX, texY, e.animFrame);
                else if (e.type==ENT_ITEM)
                    c = item_sprite_pixel(e.itemType, texX, texY);

                if (c.a==0) continue;
                c = apply_lighting(c, e.pos, transY, false);

                put_pixel(x, y, c);
                if (lodStep==2) {
                    put_pixel(x+1, y,   c);
                    put_pixel(x,   y+1, c);
                    put_pixel(x+1, y+1, c);
                }
            }
        }
    }
}

static void render_crosshair() {
    int cx=SCREEN_W/2, cy=SCREEN_H/2;
    Color dot  = {255,80,80,255};
    Color line = {200,200,200,200};
    for (int i=3;i<=7;i++) {
        put_pixel(cx+i, cy, line);
        put_pixel(cx-i, cy, line);
        put_pixel(cx, cy+i, line);
        put_pixel(cx, cy-i, line);
    }
    put_pixel(cx,   cy,   dot);
    put_pixel(cx+1, cy,   dot);
    put_pixel(cx-1, cy,   dot);
    put_pixel(cx,   cy+1, dot);
    put_pixel(cx,   cy-1, dot);
}

static void render_damage_flash(double flash) {
    if (flash <= 0) return;
    uint8_t a = (uint8_t)(flash * 80);
    for (int y=0;y<SCREEN_H;y++)
    for (int x=0;x<SCREEN_W;x++) {
        uint32_t& px = g_pixels[y*SCREEN_W+x];
        uint8_t r = (px>>16)&0xFF;
        uint8_t g = (px>>8)&0xFF;
        uint8_t b = px&0xFF;
        r = (uint8_t)std::min(255, r + a);
        px = 0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
    }
}

static void render_pickup_flash(double flash) {
    if (flash <= 0) return;
    uint8_t a = (uint8_t)(flash * 40);
    for (int y=0;y<SCREEN_H;y++)
    for (int x=0;x<SCREEN_W;x++) {
        uint32_t& px = g_pixels[y*SCREEN_W+x];
        uint8_t r = (px>>16)&0xFF;
        uint8_t g = (px>>8)&0xFF;
        uint8_t b = px&0xFF;
        g = (uint8_t)std::min(255, g + a);
        px = 0xFF000000u|((uint32_t)r<<16)|((uint32_t)g<<8)|b;
    }
}

inline void renderer_clear() {
    uint32_t sky = 0xFF000000u|((uint32_t)FOG_COLOR.r<<16)|((uint32_t)FOG_COLOR.g<<8)|FOG_COLOR.b;
    for (int i=0;i<SCREEN_W*SCREEN_H;i++) g_pixels[i]=sky;
}

inline void renderer_draw_frame(SDL_Renderer* rend, const Player& p) {
    renderer_clear();
    render_floor_ceil(p);
    render_walls(p);
    render_sprites(p);
    render_crosshair();
    if (p.damageFlash > 0) render_damage_flash(p.damageFlash);
    if (p.pickupFlash > 0) render_pickup_flash(p.pickupFlash);
    SDL_UpdateTexture(g_sdlTex, nullptr, g_pixels, SCREEN_W*4);
    SDL_RenderCopy(rend, g_sdlTex, nullptr, nullptr);
}
