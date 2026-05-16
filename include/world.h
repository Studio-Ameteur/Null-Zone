#pragma once
#include "common.h"

enum TileType {
    TILE_EMPTY = 0,
    TILE_WALL,
    TILE_DOOR,
    TILE_EXIT
};

struct Tile {
    TileType type;
    int      texWall;
    int      texFloor;
    int      texCeil;
    bool     visited;
    bool     doorOpen;
    double   doorTimer;
};

enum RoomState {
    ROOM_UNLOADED,
    ROOM_ADJACENT,
    ROOM_VISIBLE
};

struct Room {
    int        x, y, w, h;
    RoomState  state;
    int        lightCount;
    PointLight lights[4];
};

struct World {
    Tile     map[MAP_MAX][MAP_MAX];
    int      width;
    int      height;
    Vec2     playerStart;
    Vec2     exitPos;
    int      roomCount;
    Room     rooms[64];
    uint32_t seed;
    int      level;

    struct SpatialCell {
        int entities[16];
        int count;
    };
    SpatialCell spatial[MAP_MAX/SPATIAL_CELL+2][MAP_MAX/SPATIAL_CELL+2];
};

static World g_world = {};

inline bool world_in_bounds(int x, int y) {
    return x >= 0 && y >= 0 && x < g_world.width && y < g_world.height;
}

inline bool world_is_solid(int x, int y) {
    if (!world_in_bounds(x, y)) return true;
    TileType t = g_world.map[y][x].type;
    return t == TILE_WALL || (t == TILE_DOOR && !g_world.map[y][x].doorOpen);
}

inline bool world_is_solid_f(double x, double y) {
    return world_is_solid((int)x, (int)y);
}

inline void world_spatial_clear() {
    int cw = g_world.width  / SPATIAL_CELL + 2;
    int ch = g_world.height / SPATIAL_CELL + 2;
    for (int y=0; y<ch; y++)
    for (int x=0; x<cw; x++)
        g_world.spatial[y][x].count = 0;
}

inline void world_spatial_insert(int entIdx, Vec2 pos) {
    int cx = (int)(pos.x / SPATIAL_CELL);
    int cy = (int)(pos.y / SPATIAL_CELL);
    cx = std::max(0, std::min(MAP_MAX/SPATIAL_CELL+1, cx));
    cy = std::max(0, std::min(MAP_MAX/SPATIAL_CELL+1, cy));
    World::SpatialCell& cell = g_world.spatial[cy][cx];
    if (cell.count < 16)
        cell.entities[cell.count++] = entIdx;
}

inline void world_update_doors(double dt) {
    for (int y=0; y<g_world.height; y++)
    for (int x=0; x<g_world.width;  x++) {
        Tile& t = g_world.map[y][x];
        if (t.type != TILE_DOOR) continue;
        if (t.doorOpen) {
            t.doorTimer -= dt;
            if (t.doorTimer <= 0.0) {
                t.doorOpen  = false;
                t.doorTimer = 0.0;
            }
        }
    }
}

inline void world_open_door(int x, int y) {
    if (!world_in_bounds(x, y)) return;
    Tile& t = g_world.map[y][x];
    if (t.type == TILE_DOOR) {
        t.doorOpen  = true;
        t.doorTimer = 4.0;
    }
}

inline void world_update_room_states(Vec2 playerPos) {
    for (int i=0; i<g_world.roomCount; i++) {
        Room& r = g_world.rooms[i];
        double cx = r.x + r.w * 0.5;
        double cy = r.y + r.h * 0.5;
        double dx = cx - playerPos.x;
        double dy = cy - playerPos.y;
        double dist = sqrt(dx*dx + dy*dy);
        if      (dist < AGGRO_RADIUS)    r.state = ROOM_VISIBLE;
        else if (dist < ADJACENT_RADIUS) r.state = ROOM_ADJACENT;
        else                             r.state = ROOM_UNLOADED;
    }
}

inline void world_update_lights(double dt) {
    for (int i=0; i<g_world.roomCount; i++) {
        Room& r = g_world.rooms[i];
        if (r.state == ROOM_UNLOADED) continue;
        for (int li=0; li<r.lightCount; li++) {
            PointLight& pl = r.lights[li];
            if (!pl.active || pl.flicker <= 0) continue;
            pl.flickerTimer -= dt;
            if (pl.flickerTimer <= 0) {
                pl.intensity    = 0.5 + ((double)(rand()%100)/100.0) * 0.8;
                pl.flickerTimer = 0.05 + ((double)(rand()%10)/100.0);
            }
        }
    }
}
