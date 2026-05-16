#pragma once
#include <SDL.h>
#include "common.h"
#include "world.h"
#include "input.h"

static const char* weapon_names[MAX_WEAPONS] = {
    "FISTS", "PISTOL", "SHOTGUN", "AUTO RIFLE", "SNIPER RIFLE",
    "ROCKET LAUNCHER", "PLASMA GUN", "SUPER SHOTGUN", "FLAMETHROWER",
    "RAILGUN", "GRENADE LAUNCHER", "MINIGUN", "CRYO CANNON",
    "CHAINSAW", "BFG-9000"
};

static const int weapon_ammo_max[MAX_WEAPONS] = {
    0, 200, 80, 300, 30, 20, 150, 60, 200, 20, 40, 600, 100, 0, 5
};
static const int weapon_damage[MAX_WEAPONS] = {
    15, 20, 80, 25, 120, 200, 60, 130, 15, 250, 120, 18, 40, 30, 9999
};
static const double weapon_firerate[MAX_WEAPONS] = {
    0.4, 0.5, 0.9, 0.12, 1.2, 0.8, 0.25, 1.0, 0.08, 1.5, 0.7, 0.07, 0.3, 0.15, 3.0
};
static const double weapon_spread[MAX_WEAPONS] = {
    0, 0.01, 0.15, 0.06, 0.002, 0.05, 0.02, 0.2, 0.1, 0.001, 0.08, 0.08, 0.05, 0, 0
};
static const bool weapon_auto[MAX_WEAPONS] = {
    false,false,false,true,false,false,true,false,true,false,false,true,true,true,false
};

struct Player {
    Vec2   pos;
    double angle;
    Vec2   dir;
    Vec2   plane;

    int    health;
    int    maxHealth;
    int    armor;
    int    maxArmor;
    int    score;
    int    kills;
    int    level;
    bool   alive;

    bool   hackerEffect;
    double hackerTimer;

    double bobTimer;
    double bobAmt;
    double bobSide;

    int    currentWeapon;
    Weapon weapons[MAX_WEAPONS];

    double footstepTimer;
    double footstepInterval;

    double damageFlash;
    double pickupFlash;
};

static Player g_player = {};

inline void player_init_weapons(Player& p) {
    for (int i=0; i<MAX_WEAPONS; i++) {
        p.weapons[i].type       = (WeaponType)i;
        p.weapons[i].owned      = (i==0 || i==1);
        p.weapons[i].ammo       = (i==1) ? 50 : 0;
        p.weapons[i].maxAmmo    = weapon_ammo_max[i];
        p.weapons[i].damage     = weapon_damage[i];
        p.weapons[i].fireRate   = weapon_firerate[i];
        p.weapons[i].fireTimer  = 0.0;
        p.weapons[i].spread     = weapon_spread[i];
        p.weapons[i].automatic  = weapon_auto[i];
        p.weapons[i].spriteIndex= i;
    }
    p.currentWeapon = 1;
}

inline void player_init(Player& p, Vec2 startPos, int level) {
    p.pos              = startPos;
    p.angle            = 0.0;
    p.dir              = {1.0, 0.0};
    p.plane            = {0.0, tan(FOV * 0.5)};
    p.health           = 100;
    p.maxHealth        = 100;
    p.armor            = 0;
    p.maxArmor         = 200;
    p.score            = 0;
    p.kills            = 0;
    p.level            = level;
    p.alive            = true;
    p.hackerEffect     = false;
    p.hackerTimer      = 0.0;
    p.bobTimer         = 0.0;
    p.bobAmt           = 0.0;
    p.bobSide          = 0.0;
    p.footstepTimer    = 0.0;
    p.footstepInterval = 0.4;
    p.damageFlash      = 0.0;
    p.pickupFlash      = 0.0;
    player_init_weapons(p);
}

inline void player_update_dir(Player& p) {
    p.dir   = { cos(p.angle), sin(p.angle) };
    p.plane = { -sin(p.angle)*tan(FOV*0.5),
                 cos(p.angle)*tan(FOV*0.5) };
}

inline bool player_try_move(Player& p, double dx, double dy) {
    const double R = 0.25;
    double nx = p.pos.x + dx;
    double ny = p.pos.y + dy;
    bool okX = !world_is_solid((int)(nx+R),(int)(p.pos.y+R)) &&
               !world_is_solid((int)(nx+R),(int)(p.pos.y-R)) &&
               !world_is_solid((int)(nx-R),(int)(p.pos.y+R)) &&
               !world_is_solid((int)(nx-R),(int)(p.pos.y-R));
    bool okY = !world_is_solid((int)(p.pos.x+R),(int)(ny+R)) &&
               !world_is_solid((int)(p.pos.x+R),(int)(ny-R)) &&
               !world_is_solid((int)(p.pos.x-R),(int)(ny+R)) &&
               !world_is_solid((int)(p.pos.x-R),(int)(ny-R));
    if (okX) p.pos.x = nx;
    if (okY) p.pos.y = ny;
    return okX || okY;
}

inline void player_switch_weapon(Player& p, int dir) {
    int cur = p.currentWeapon;
    for (int i=1; i<MAX_WEAPONS; i++) {
        int next = ((cur + dir*i) % MAX_WEAPONS + MAX_WEAPONS) % MAX_WEAPONS;
        if (p.weapons[next].owned) {
            p.currentWeapon = next;
            break;
        }
    }
}

inline void player_take_damage(Player& p, int dmg) {
    if (p.armor > 0) {
        int absorbed = std::min(p.armor, dmg/2);
        p.armor  -= absorbed;
        dmg      -= absorbed;
    }
    p.health      -= dmg;
    p.damageFlash  = 0.3;
    if (p.health <= 0) { p.health = 0; p.alive = false; }
}

inline void player_heal(Player& p, int amt) {
    p.health      = std::min(p.maxHealth, p.health + amt);
    p.pickupFlash = 0.2;
}

inline void player_add_armor(Player& p, int amt) {
    p.armor       = std::min(p.maxArmor, p.armor + amt);
    p.pickupFlash = 0.2;
}

inline void player_add_ammo(Player& p, WeaponType wt, int amt) {
    Weapon& w     = p.weapons[(int)wt];
    w.ammo        = std::min(w.maxAmmo, w.ammo + amt);
    p.pickupFlash = 0.15;
}

inline bool player_needs_footstep(Player& p, double dt, bool moving) {
    if (!moving) { p.footstepTimer = 0; return false; }
    p.footstepTimer += dt;
    if (p.footstepTimer >= p.footstepInterval) {
        p.footstepTimer = 0.0;
        return true;
    }
    return false;
}

inline void player_update(Player& p, double dt, bool& footstep) {
    if (!p.alive) return;

    double rotSpeed = ROT_SPEED * mouse_dx();
    if (p.hackerEffect) rotSpeed *= -1.5;
    p.angle += rotSpeed;
    player_update_dir(p);

    double ms = MOVE_SPEED;
    bool moving = false;

    if (move_forward())  { player_try_move(p,  p.dir.x*ms,  p.dir.y*ms); moving=true; }
    if (move_backward()) { player_try_move(p, -p.dir.x*ms, -p.dir.y*ms); moving=true; }
    if (move_left())     { player_try_move(p,  p.dir.y*ms, -p.dir.x*ms); moving=true; }
    if (move_right())    { player_try_move(p, -p.dir.y*ms,  p.dir.x*ms); moving=true; }

    if (moving) {
        p.bobTimer += dt * 8.0;
        p.bobAmt    = sin(p.bobTimer) * 0.03;
        p.bobSide   = sin(p.bobTimer * 0.5) * 0.015;
    } else {
        p.bobTimer *= 0.85;
        p.bobAmt   *= 0.85;
        p.bobSide  *= 0.85;
    }

    footstep = player_needs_footstep(p, dt, moving);

    if (p.hackerEffect) {
        p.hackerTimer -= dt;
        if (p.hackerTimer <= 0.0) p.hackerEffect = false;
    }

    if (p.damageFlash > 0) p.damageFlash -= dt;
    if (p.pickupFlash > 0) p.pickupFlash -= dt;

    Weapon& w = p.weapons[p.currentWeapon];
    if (w.fireTimer > 0) w.fireTimer -= dt;

    int wd = wheel();
    if (wd != 0) player_switch_weapon(p, wd > 0 ? 1 : -1);

    for (int i=0; i<9; i++) {
        if (weapon_key(i) && i < MAX_WEAPONS && p.weapons[i].owned)
            p.currentWeapon = i;
    }

    if (action_use()) {
        int fx = (int)(p.pos.x + p.dir.x * 1.5);
        int fy = (int)(p.pos.y + p.dir.y * 1.5);
        world_open_door(fx, fy);
    }
}

inline bool player_can_fire(Player& p) {
    Weapon& w = p.weapons[p.currentWeapon];
    if (w.fireTimer > 0) return false;
    if (p.currentWeapon == (int)WPN_FISTS || p.currentWeapon == (int)WPN_CHAINSAW)
        return fire();
    if (w.automatic) return fire();
    return fire_pressed();
}

inline bool player_do_fire(Player& p) {
    Weapon& w = p.weapons[p.currentWeapon];
    bool isMelee = (p.currentWeapon == (int)WPN_FISTS ||
                    p.currentWeapon == (int)WPN_CHAINSAW);
    if (!isMelee && w.ammo <= 0) return false;
    if (!isMelee) w.ammo--;
    w.fireTimer = w.fireRate;
    return true;
}

inline PlayerStats player_get_stats(const Player& p) {
    PlayerStats s;
    s.health        = p.health;
    s.maxHealth     = p.maxHealth;
    s.armor         = p.armor;
    s.maxArmor      = p.maxArmor;
    s.score         = p.score;
    s.kills         = p.kills;
    s.level         = p.level;
    s.pos           = p.pos;
    s.angle         = p.angle;
    s.currentWeapon = p.currentWeapon;
    for (int i=0; i<MAX_WEAPONS; i++) s.weapons[i] = p.weapons[i];
    return s;
}
