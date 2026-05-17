#pragma once
#include "world.h"

struct BSPNode {
    int  x, y, w, h;
    int  left, right;
    bool isLeaf;
    int  roomX, roomY, roomW, roomH;
    int  zoneStyle;
};

static BSPNode  g_bsp[256];
static int      g_bspCount = 0;
static uint32_t g_rng      = 0;

static inline uint32_t rng_next() {
    g_rng ^= g_rng << 13;
    g_rng ^= g_rng >> 17;
    g_rng ^= g_rng << 5;
    return g_rng;
}

static inline int rng_range(int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + (int)(rng_next() % (uint32_t)(hi - lo));
}

static inline double rng_float() {
    return (double)(rng_next() & 0xFFFF) / 65535.0;
}

static void tex_for_zone(int zone, int& wall, int& floor, int& ceil) {
    if (zone == 0) {
        int walls[]  = {0,1,2,3,4,5};
        int floors[] = {12,13,14};
        int ceils[]  = {16,17};
        wall  = walls [rng_range(0,6)];
        floor = floors[rng_range(0,3)];
        ceil  = ceils [rng_range(0,2)];
    } else {
        int walls[]  = {6,7,8,9,10,11};
        int floors[] = {15,13};
        int ceils[]  = {18,19};
        wall  = walls [rng_range(0,6)];
        floor = floors[rng_range(0,2)];
        ceil  = ceils [rng_range(0,2)];
    }
}

static void bsp_split(int idx, int minSize) {
    BSPNode& node = g_bsp[idx];
    bool canH = node.h >= minSize*2;
    bool canV = node.w >= minSize*2;
    if (!canH && !canV) { node.isLeaf=true; return; }

    bool doH;
    if  (canH && !canV) doH=true;
    else if (!canH && canV) doH=false;
    else doH=(rng_next()%2==0);

    if (doH) {
        int split = rng_range(minSize, node.h-minSize);
        int li=g_bspCount++, ri=g_bspCount++;
        g_bsp[li]={node.x,node.y,        node.w,split,          -1,-1,false,0,0,0,0,node.zoneStyle};
        g_bsp[ri]={node.x,node.y+split,  node.w,node.h-split,   -1,-1,false,0,0,0,0,node.zoneStyle^1};
        node.left=li; node.right=ri; node.isLeaf=false;
        bsp_split(li,minSize); bsp_split(ri,minSize);
    } else {
        int split = rng_range(minSize, node.w-minSize);
        int li=g_bspCount++, ri=g_bspCount++;
        g_bsp[li]={node.x,      node.y, split,        node.h, -1,-1,false,0,0,0,0,node.zoneStyle};
        g_bsp[ri]={node.x+split,node.y, node.w-split, node.h, -1,-1,false,0,0,0,0,node.zoneStyle^1};
        node.left=li; node.right=ri; node.isLeaf=false;
        bsp_split(li,minSize); bsp_split(ri,minSize);
    }
}

static void bsp_place_rooms(int idx) {
    BSPNode& node = g_bsp[idx];
    if (node.isLeaf) {
        int margin=2;
        int rw=rng_range(4, std::max(4,node.w-margin*2));
        int rh=rng_range(4, std::max(4,node.h-margin*2));
        int rx=node.x+margin+rng_range(0,std::max(0,node.w-rw-margin*2));
        int ry=node.y+margin+rng_range(0,std::max(0,node.h-rh-margin*2));
        node.roomX=rx; node.roomY=ry; node.roomW=rw; node.roomH=rh;

        int wallTex, floorTex, ceilTex;
        tex_for_zone(node.zoneStyle, wallTex, floorTex, ceilTex);

        for(int y=ry;y<ry+rh&&y<g_world.height;y++)
        for(int x=rx;x<rx+rw&&x<g_world.width; x++) {
            bool border=(x==rx||x==rx+rw-1||y==ry||y==ry+rh-1);
            Tile& t=g_world.map[y][x];
            t.type    =border?TILE_WALL:TILE_EMPTY;
            t.texWall =wallTex;
            t.texFloor=floorTex;
            t.texCeil =ceilTex;
            t.visited =false;
            t.doorOpen=false;
            t.doorTimer=0.0;
        }

        if(g_world.roomCount<64) {
            Room& r=g_world.rooms[g_world.roomCount++];
            r.x=rx; r.y=ry; r.w=rw; r.h=rh;
            r.state=ROOM_UNLOADED;
            r.lightCount=0;

            int numLights=rng_range(1,4);
            for(int i=0;i<numLights&&i<4;i++) {
                PointLight& pl=r.lights[r.lightCount++];
                pl.pos={(double)(rx+rng_range(1,rw-1))+0.5,
                        (double)(ry+rng_range(1,rh-1))+0.5};
                if(node.zoneStyle==0) {
                    double roll=rng_float();
                    if     (roll<0.5) pl.color={80,120,200,255};
                    else if(roll<0.8) pl.color={100,150,220,255};
                    else              pl.color={60,200,200,255};
                } else {
                    double roll=rng_float();
                    if     (roll<0.4) pl.color={200,100,20,255};
                    else if(roll<0.7) pl.color={200,50,20,255};
                    else              pl.color={220,180,20,255};
                }
                pl.radius     =(double)rng_range(4,10);
                pl.intensity  =0.6+rng_float()*0.8;
                pl.active     =true;
                pl.flicker    =rng_float()<0.3?0.08:0.0;
                pl.flickerTimer=0.0;
            }
        }
        return;
    }
    if(node.left >=0) bsp_place_rooms(node.left);
    if(node.right>=0) bsp_place_rooms(node.right);
}

static void bsp_carve_corridor(int ax,int ay,int bx,int by,int zone) {
    int wallTex, floorTex, ceilTex;
    tex_for_zone(zone, wallTex, floorTex, ceilTex);
    int cx=ax,cy=ay;
    while(cx!=bx) {
        if(world_in_bounds(cx,cy)) {
            Tile& t=g_world.map[cy][cx];
            if(t.type==TILE_WALL) {
                t.type    =(rng_next()%12==0)?TILE_DOOR:TILE_EMPTY;
                t.texWall =wallTex;
                t.texFloor=floorTex;
                t.texCeil =ceilTex;
            }
        }
        cx+=(bx>cx)?1:-1;
    }
    while(cy!=by) {
        if(world_in_bounds(cx,cy)) {
            Tile& t=g_world.map[cy][cx];
            if(t.type==TILE_WALL) {
                t.type    =(rng_next()%12==0)?TILE_DOOR:TILE_EMPTY;
                t.texWall =wallTex;
                t.texFloor=floorTex;
                t.texCeil =ceilTex;
            }
        }
        cy+=(by>cy)?1:-1;
    }
}

static void bsp_connect(int idx) {
    BSPNode& node=g_bsp[idx];
    if(node.isLeaf) return;
    bsp_connect(node.left);
    bsp_connect(node.right);

    BSPNode* la=&g_bsp[node.left];
    BSPNode* ra=&g_bsp[node.right];
    while(!la->isLeaf) la=&g_bsp[la->left];
    while(!ra->isLeaf) ra=&g_bsp[ra->left];

    int ax=la->roomX+la->roomW/2, ay=la->roomY+la->roomH/2;
    int bx=ra->roomX+ra->roomW/2, by=ra->roomY+ra->roomH/2;
    bsp_carve_corridor(ax,ay,bx,by,node.zoneStyle);
}

static int bsp_first_leaf(int idx) {
    while(!g_bsp[idx].isLeaf) idx=g_bsp[idx].left;
    return idx;
}

static int bsp_last_leaf(int idx) {
    while(!g_bsp[idx].isLeaf) idx=g_bsp[idx].right;
    return idx;
}

inline void world_generate(int level) {
    uint32_t seed=(uint32_t)(level*1234567891u+0xCAFEBABEu);
    g_rng=seed;
    g_world.seed=seed;
    g_world.level=level;
    g_world.roomCount=0;

    double t=(double)level/(double)MAX_LEVELS;
    int mapSize=(int)(32+t*30);
    mapSize=std::min(mapSize,MAP_MAX-2);
    g_world.width=mapSize;
    g_world.height=mapSize;

    for(int y=0;y<MAP_MAX;y++)
    for(int x=0;x<MAP_MAX;x++)
        g_world.map[y][x]={TILE_WALL,6,15,18,false,false,0.0};

    g_bspCount=0;
    memset(g_bsp,0,sizeof(g_bsp));
    int startZone=(level%2==0)?0:1;
    g_bsp[0]={1,1,mapSize-2,mapSize-2,-1,-1,false,0,0,0,0,startZone};
    g_bspCount=1;

    bsp_split(0,6);
    bsp_place_rooms(0);
    bsp_connect(0);

    int startIdx=bsp_first_leaf(0);
    int exitIdx =bsp_last_leaf(0);

    BSPNode& sn=g_bsp[startIdx];
    BSPNode& en=g_bsp[exitIdx];

    g_world.playerStart={(double)(sn.roomX+sn.roomW/2)+0.5,
                         (double)(sn.roomY+sn.roomH/2)+0.5};
    g_world.exitPos    ={(double)(en.roomX+en.roomW/2)+0.5,
                         (double)(en.roomY+en.roomH/2)+0.5};

    int ex=(int)g_world.exitPos.x, ey=(int)g_world.exitPos.y;
    if(world_in_bounds(ex,ey)) g_world.map[ey][ex].type=TILE_EXIT;

    world_spatial_clear();
}
