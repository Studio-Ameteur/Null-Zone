#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

#define NZ_VERSION "0.0.1"
#define NZ_TITLE   "Null Zone"
#define NZ_AUTHOR  "Savva Polyakov"
#define NZ_EMAIL   "solo2244rt@gmail.com"
#define NZ_YEAR    "2026"

static const int    SCREEN_W         = 1280;
static const int    SCREEN_H         = 720;
static const int    HALF_H           = SCREEN_H / 2;
static const double FOV              = 1.0472;
static const int    NUM_RAYS         = SCREEN_W;
static const double MOVE_SPEED       = 0.055;
static const double ROT_SPEED        = 0.038;
static const int    MAP_MAX          = 64;
static const double EPSILON          = 1e-9;
static const int    MAX_LIGHTS       = 32;
static const int    MAX_ENTITIES     = 256;
static const int    TEX_SIZE         = 64;
static const int    NUM_TEXTURES     = 23;
static const int    AGGRO_RADIUS     = 14;
static const int    ADJACENT_RADIUS  = 22;
static const int    MAX_WEAPONS      = 15;
static const int    MAX_SAVE_SLOTS   = 8;
static const int    FIXED_TICKS      = 64;
static const int    SPATIAL_CELL     = 4;
static const int    MAX_LEVELS       = 200;

struct Vec2 {
    double x, y;
    Vec2(double x=0, double y=0) : x(x), y(y) {}
    Vec2 operator+(const Vec2& o) const { return {x+o.x, y+o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x-o.x, y-o.y}; }
    Vec2 operator*(double s)      const { return {x*s,   y*s};   }
    double dot(const Vec2& o)     const { return x*o.x + y*o.y;  }
    double length()               const { return sqrt(x*x+y*y);  }
    Vec2 normalized() const {
        double l = length();
        return l > EPSILON ? Vec2(x/l, y/l) : Vec2(0, 0);
    }
    Vec2 rotated(double a) const {
        return { x*cos(a)-y*sin(a), x*sin(a)+y*cos(a) };
    }
    double distTo(const Vec2& o) const {
        double dx=x-o.x, dy=y-o.y;
        return sqrt(dx*dx+dy*dy);
    }
};

struct Color {
    uint8_t r, g, b, a;
    Color(uint8_t r=0, uint8_t g=0, uint8_t b=0, uint8_t a=255)
        : r(r), g(g), b(b), a(a) {}
    Color operator*(double f) const {
        return {
            (uint8_t)std::min(255.0, r*f),
            (uint8_t)std::min(255.0, g*f),
            (uint8_t)std::min(255.0, b*f),
            a
        };
    }
    Color blend(const Color& o, double t) const {
        t = std::max(0.0, std::min(1.0, t));
        return {
            (uint8_t)(r*(1-t) + o.r*t),
            (uint8_t)(g*(1-t) + o.g*t),
            (uint8_t)(b*(1-t) + o.b*t),
            a
        };
    }
    Color operator+(const Color& o) const {
        return {
            (uint8_t)std::min(255, (int)r+(int)o.r),
            (uint8_t)std::min(255, (int)g+(int)o.g),
            (uint8_t)std::min(255, (int)b+(int)o.b),
            a
        };
    }
};

struct PointLight {
    Vec2   pos;
    Color  color;
    double radius;
    double intensity;
    bool   active;
    double flicker;
    double flickerTimer;
};

enum EntityType  { ENT_NONE, ENT_ENEMY, ENT_ITEM, ENT_DECORATION };
enum EntityState { ES_IDLE, ES_ALERT, ES_CHASE, ES_ATTACK, ES_DEAD };

enum ItemType {
    ITEM_NONE,
    ITEM_HEALTH_SMALL,
    ITEM_HEALTH_BIG,
    ITEM_ARMOR,
    ITEM_AMMO_PISTOL,
    ITEM_AMMO_SHOTGUN,
    ITEM_AMMO_AUTO,
    ITEM_AMMO_ROCKET,
    ITEM_AMMO_PLASMA,
    ITEM_AMMO_RAIL,
    ITEM_AMMO_GRENADE,
    ITEM_AMMO_MINIGUN,
    ITEM_AMMO_CRYO,
    ITEM_AMMO_FLAME,
    ITEM_WEAPON
};

enum EnemyType {
    ENEMY_DRONE,
    ENEMY_CRAWLER,
    ENEMY_BRUTE,
    ENEMY_PHANTOM,
    ENEMY_TURRET,
    ENEMY_SWARM,
    ENEMY_STALKER,
    ENEMY_BOMBER,
    ENEMY_SNIPER,
    ENEMY_SHIELD,
    ENEMY_HEALER,
    ENEMY_GHOST,
    ENEMY_JUGGERNAUT,
    ENEMY_HACKER,
    ENEMY_BOSS
};

enum WeaponType {
    WPN_FISTS,
    WPN_PISTOL,
    WPN_SHOTGUN,
    WPN_AUTO,
    WPN_SNIPER,
    WPN_ROCKET,
    WPN_PLASMA,
    WPN_SUPER_SHOTGUN,
    WPN_FLAMETHROWER,
    WPN_RAILGUN,
    WPN_GRENADE,
    WPN_MINIGUN,
    WPN_CRYO,
    WPN_CHAINSAW,
    WPN_BFG
};

struct Weapon {
    WeaponType type;
    bool       owned;
    int        ammo;
    int        maxAmmo;
    int        damage;
    double     fireRate;
    double     fireTimer;
    double     spread;
    bool       automatic;
    int        spriteIndex;
};

struct Entity {
    EntityType  type;
    EntityState state;
    EnemyType   enemyType;
    ItemType    itemType;
    WeaponType  weaponType;
    Vec2        pos;
    double      angle;
    int         health;
    int         maxHealth;
    int         spriteIndex;
    double      dist;
    bool        active;
    bool        frozen;
    double      animTimer;
    int         animFrame;
    int         animFrameCount;
    Vec2        lastPlayerPos;
    double      alertTimer;
    double      attackTimer;
    double      speed;
    int         damage;
    bool        throughWalls;
    bool        invisible;
    bool        inFOV;
    bool        inAggro;
};

struct RayHit {
    double dist;
    double wallX;
    int    texIndex;
    bool   side;
    int    mapX, mapY;
};

struct SaveHeader {
    uint8_t  magic[4];
    uint8_t  uuid[16];
    uint32_t crc32;
    uint32_t level;
    uint32_t health;
    uint32_t armor;
    uint32_t score;
    uint32_t kills;
    uint32_t timestamp;
    uint8_t  slotType;
    char     padding[3];
};

struct RegistryEntry {
    uint8_t  uuid[16];
    uint8_t  slotType;
    char     padding[3];
    uint32_t timestamp;
    uint32_t fileCRC;
    uint32_t level;
    uint32_t score;
};

enum GameState {
    GS_MENU,
    GS_PLAYING,
    GS_PAUSED,
    GS_DEAD,
    GS_LEVEL_COMPLETE,
    GS_SAVE_MENU,
    GS_LOAD_MENU,
    GS_SETTINGS
};

struct PlayerStats {
    int    health;
    int    maxHealth;
    int    armor;
    int    maxArmor;
    int    score;
    int    kills;
    int    level;
    Vec2   pos;
    double angle;
    int    currentWeapon;
    Weapon weapons[MAX_WEAPONS];
};

struct DebugInfo {
    double   fps;
    double   frametime;
    int      visibleEntities;
    int      frozenEntities;
    int      activeRooms;
    int      totalEntities;
    int      aggroEntities;
    unsigned memUsageKB;
    int      level;
    uint32_t seed;
    double   playerX;
    double   playerY;
    double   playerAngle;
    int      currentWeapon;
    int      currentAmmo;
};

inline uint32_t crc32_compute(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320u & (uint32_t)(-(int32_t)(crc & 1)));
    }
    return ~crc;
}

inline void uuid_generate(uint8_t out[16], uint32_t seed_extra) {
    static uint32_t seed = 0xDEADBEEF;
    seed ^= seed_extra;
    for (int i = 0; i < 16; i++) {
        seed ^= seed << 13;
        seed ^= seed >> 17;
        seed ^= seed << 5;
        out[i] = (uint8_t)(seed & 0xFF);
    }
    out[6] = (out[6] & 0x0F) | 0x40;
    out[8] = (out[8] & 0x3F) | 0x80;
}

inline bool uuid_equal(const uint8_t a[16], const uint8_t b[16]) {
    return memcmp(a, b, 16) == 0;
}
