#pragma once
#include "common.h"
#include "world.h"
#include "player.h"

static Entity  g_entities[MAX_ENTITIES];
static int     g_entityCount = 0;
static uint32_t g_ent_rng   = 0;

static inline uint32_t ent_rng_next() {
    g_ent_rng ^= g_ent_rng << 13;
    g_ent_rng ^= g_ent_rng >> 17;
    g_ent_rng ^= g_ent_rng << 5;
    return g_ent_rng;
}
static inline int ent_rng_range(int lo, int hi) {
    if (lo >= hi) return lo;
    return lo + (int)(ent_rng_next() % (uint32_t)(hi-lo));
}
static inline double ent_rng_float() {
    return (double)(ent_rng_next() & 0xFFFF) / 65535.0;
}

static const int enemy_hp[15] = {
    60,40,300,80,200,20,90,50,100,150,70,120,500,80,1000
};
static const double enemy_spd[15] = {
    0.03,0.06,0.02,0.04,0.0,0.07,0.035,0.08,0.01,0.025,0.02,0.04,0.015,0.0,0.02
};
static const int enemy_dmg[15] = {
    15,20,50,25,30,8,20,120,40,35,10,30,80,25,150
};
static const double enemy_atk_rate[15] = {
    1.5,0.8,2.0,1.2,0.6,0.5,1.0,0.0,2.5,1.8,3.0,1.5,2.5,0.0,1.0
};

inline void entities_clear() {
    g_entityCount = 0;
    memset(g_entities, 0, sizeof(g_entities));
}

inline int entity_spawn(EntityType type, Vec2 pos) {
    if (g_entityCount >= MAX_ENTITIES) return -1;
    int i = g_entityCount++;
    memset(&g_entities[i], 0, sizeof(Entity));
    g_entities[i].type        = type;
    g_entities[i].pos         = pos;
    g_entities[i].active      = true;
    g_entities[i].frozen      = true;
    g_entities[i].state       = ES_IDLE;
    g_entities[i].inFOV       = false;
    g_entities[i].inAggro     = false;
    return i;
}

inline int entity_spawn_enemy(EnemyType et, Vec2 pos) {
    int i = entity_spawn(ENT_ENEMY, pos);
    if (i < 0) return -1;
    Entity& e           = g_entities[i];
    e.enemyType         = et;
    e.health            = enemy_hp[et];
    e.maxHealth         = enemy_hp[et];
    e.speed             = enemy_spd[et];
    e.damage            = enemy_dmg[et];
    e.attackTimer       = 0.0;
    e.alertTimer        = 0.0;
    e.spriteIndex       = (int)et;
    e.throughWalls      = (et == ENEMY_GHOST);
    e.invisible         = (et == ENEMY_PHANTOM);
    e.animFrameCount    = 4;
    e.animFrame         = 0;
    e.animTimer         = 0.0;
    return i;
}

inline int entity_spawn_item(ItemType it, Vec2 pos, WeaponType wt=WPN_FISTS) {
    int i = entity_spawn(ENT_ITEM, pos);
    if (i < 0) return -1;
    g_entities[i].itemType   = it;
    g_entities[i].weaponType = wt;
    g_entities[i].spriteIndex= 20 + (int)it;
    return i;
}

static bool los_check(Vec2 from, Vec2 to) {
    double dx   = to.x - from.x;
    double dy   = to.y - from.y;
    double dist = sqrt(dx*dx + dy*dy);
    if (dist < EPSILON) return true;
    int steps = (int)(dist * 4.0);
    if (steps < 2) steps = 2;
    for (int i=1; i<steps; i++) {
        double t = (double)i / (double)steps;
        if (world_is_solid((int)(from.x+dx*t), (int)(from.y+dy*t)))
            return false;
    }
    return true;
}

static void enemy_ai_tick(Entity& e, Vec2 ppos, double dt) {
    if (!e.active || e.frozen || e.state == ES_DEAD) return;
    double dist = e.pos.distTo(ppos);
    EnemyType et = e.enemyType;

    if (et == ENEMY_TURRET) {
        e.state = (dist < AGGRO_RADIUS && los_check(e.pos, ppos))
                  ? ES_ATTACK : ES_IDLE;
        if (e.state == ES_ATTACK)
            e.angle = atan2(ppos.y-e.pos.y, ppos.x-e.pos.x);
        e.attackTimer -= dt;
        return;
    }

    switch (e.state) {
    case ES_IDLE:
        if (dist < (double)AGGRO_RADIUS && los_check(e.pos, ppos)) {
            e.state       = ES_ALERT;
            e.alertTimer  = 0.5;
            e.lastPlayerPos = ppos;
        }
        break;

    case ES_ALERT:
        e.alertTimer -= dt;
        if (e.alertTimer <= 0) e.state = ES_CHASE;
        break;

    case ES_CHASE: {
        e.lastPlayerPos = ppos;
        double dx = ppos.x - e.pos.x;
        double dy = ppos.y - e.pos.y;
        double len = sqrt(dx*dx+dy*dy);
        if (len > EPSILON) {
            dx /= len; dy /= len;
            double nx = e.pos.x + dx*e.speed;
            double ny = e.pos.y + dy*e.speed;

            if (et == ENEMY_STALKER) {
                double side = (ent_rng_next()%2==0) ? 1.0 : -1.0;
                nx += (-dy)*e.speed*side*0.5;
                ny += ( dx)*e.speed*side*0.5;
            }

            if (e.throughWalls || !world_is_solid_f(nx, ny)) {
                e.pos = {nx, ny};
            } else {
                if (!world_is_solid_f(nx, e.pos.y)) e.pos.x = nx;
                else if (!world_is_solid_f(e.pos.x, ny)) e.pos.y = ny;
            }
        }
        double atkRange = (et==ENEMY_BOMBER) ? 1.2 : 7.0;
        if (dist < atkRange) {
            e.state       = ES_ATTACK;
            e.attackTimer = enemy_atk_rate[et];
        }
        if (dist > AGGRO_RADIUS*1.5 && !los_check(e.pos, ppos))
            e.state = ES_IDLE;
        break;
    }

    case ES_ATTACK:
        e.attackTimer -= dt;
        if (et == ENEMY_HEALER) {
            for (int i=0; i<g_entityCount; i++) {
                Entity& o = g_entities[i];
                if (&o==&e||o.type!=ENT_ENEMY||o.state==ES_DEAD||!o.active) continue;
                if (e.pos.distTo(o.pos) < 3.5)
                    o.health = std::min(o.maxHealth, o.health+2);
            }
        }
        if (e.attackTimer <= 0) {
            e.state       = ES_CHASE;
            e.attackTimer = enemy_atk_rate[et];
        }
        break;

    default: break;
    }

    e.angle = atan2(ppos.y-e.pos.y, ppos.x-e.pos.x);
    e.animTimer += dt;
    if (e.animTimer > 0.12) {
        e.animTimer = 0.0;
        e.animFrame = (e.animFrame+1) % std::max(1, e.animFrameCount);
    }
}

inline void entities_update_visibility(Vec2 playerPos, double playerAngle) {
    world_spatial_clear();
    double halfFOV = FOV * 0.6;

    for (int i=0; i<g_entityCount; i++) {
        Entity& e = g_entities[i];
        if (!e.active) continue;

        world_spatial_insert(i, e.pos);

        double dist   = e.pos.distTo(playerPos);
        e.inAggro     = (dist <= (double)AGGRO_RADIUS);
        e.frozen      = !e.inAggro;
        e.dist        = dist;

        Vec2 toEnt    = e.pos - playerPos;
        double ang    = atan2(toEnt.y, toEnt.x) - playerAngle;
        while (ang >  M_PI) ang -= 2*M_PI;
        while (ang < -M_PI) ang += 2*M_PI;
        e.inFOV = (fabs(ang) < halfFOV) && (dist < (double)ADJACENT_RADIUS);
    }
}

struct EntityHitResult {
    bool  hit;
    int   entityIdx;
    int   damage;
};

inline EntityHitResult entities_raycast_hit(Vec2 from, Vec2 dir, double maxDist, int dmg) {
    EntityHitResult res = {false, -1, 0};
    double bestDist = maxDist;
    for (int i=0; i<g_entityCount; i++) {
        Entity& e = g_entities[i];
        if (!e.active||e.type!=ENT_ENEMY||e.state==ES_DEAD||e.frozen) continue;
        Vec2 toEnt = e.pos - from;
        double proj = toEnt.dot(dir);
        if (proj < 0 || proj > bestDist) continue;
        Vec2 closest = from + dir*proj;
        double perpDist = closest.distTo(e.pos);
        if (perpDist < 0.45) {
            bestDist      = proj;
            res.hit       = true;
            res.entityIdx = i;
            res.damage    = dmg;
        }
    }
    return res;
}

inline void entity_take_damage(int idx, int dmg, Player& player) {
    if (idx < 0 || idx >= g_entityCount) return;
    Entity& e = g_entities[idx];
    if (!e.active || e.type!=ENT_ENEMY || e.state==ES_DEAD) return;
    e.health -= dmg;
    if (e.state == ES_IDLE || e.state == ES_ALERT) {
        e.state       = ES_CHASE;
        e.alertTimer  = 0.0;
    }
    if (e.health <= 0) {
        e.health = 0;
        e.state  = ES_DEAD;
        e.active = false;
        player.kills++;
        player.score += 100 * (1 + (int)e.enemyType/3);

        double roll = ent_rng_float();
        ItemType drop = ITEM_NONE;
        if      (roll < 0.25) drop = ITEM_HEALTH_SMALL;
        else if (roll < 0.35) drop = ITEM_HEALTH_BIG;
        else if (roll < 0.42) drop = ITEM_ARMOR;
        else if (roll < 0.55) drop = ITEM_AMMO_PISTOL;
        else if (roll < 0.65) drop = ITEM_AMMO_SHOTGUN;
        else if (roll < 0.75) drop = ITEM_AMMO_AUTO;
        else if (roll < 0.82) drop = ITEM_AMMO_ROCKET;
        else if (roll < 0.88) drop = ITEM_AMMO_PLASMA;

        if (drop != ITEM_NONE)
            entity_spawn_item(drop, e.pos);
    }
}

inline bool entities_update(double dt, Player& player) {
    bool playerHit   = false;
    Vec2 playerPos   = player.pos;
    double playerAng = player.angle;

    entities_update_visibility(playerPos, playerAng);

    for (int i=0; i<g_entityCount; i++) {
        Entity& e = g_entities[i];
        if (!e.active) continue;

        if (e.type == ENT_ENEMY) {
            enemy_ai_tick(e, playerPos, dt);

            if (e.state == ES_ATTACK && e.attackTimer <= 0) {
                double dist2 = e.pos.distTo(playerPos);
                double atkRange = (e.enemyType==ENEMY_BOMBER) ? 1.2 : 7.0;
                if (dist2 < atkRange && los_check(e.pos, playerPos)) {
                    player_take_damage(player, e.damage);
                    playerHit = true;
                    if (e.enemyType == ENEMY_HACKER) {
                        player.hackerEffect = true;
                        player.hackerTimer  = 5.0;
                    }
                    if (e.enemyType == ENEMY_BOMBER) {
                        e.active = false;
                    }
                    e.attackTimer = enemy_atk_rate[e.enemyType];
                }
            }
        }

        if (e.type == ENT_ITEM) {
            double dist2 = e.pos.distTo(playerPos);
            if (dist2 < 0.65) {
                bool picked = false;
                switch (e.itemType) {
                case ITEM_HEALTH_SMALL:
                    if (player.health < player.maxHealth)
                    { player_heal(player, 15); picked=true; }
                    break;
                case ITEM_HEALTH_BIG:
                    if (player.health < player.maxHealth)
                    { player_heal(player, 50); picked=true; }
                    break;
                case ITEM_ARMOR:
                    if (player.armor < player.maxArmor)
                    { player_add_armor(player, 50); picked=true; }
                    break;
                case ITEM_AMMO_PISTOL:    player_add_ammo(player,WPN_PISTOL,   30); picked=true; break;
                case ITEM_AMMO_SHOTGUN:   player_add_ammo(player,WPN_SHOTGUN,  10); picked=true; break;
                case ITEM_AMMO_AUTO:      player_add_ammo(player,WPN_AUTO,     60); picked=true; break;
                case ITEM_AMMO_ROCKET:    player_add_ammo(player,WPN_ROCKET,    5); picked=true; break;
                case ITEM_AMMO_PLASMA:    player_add_ammo(player,WPN_PLASMA,   40); picked=true; break;
                case ITEM_AMMO_RAIL:      player_add_ammo(player,WPN_RAILGUN,   5); picked=true; break;
                case ITEM_AMMO_GRENADE:   player_add_ammo(player,WPN_GRENADE,  10); picked=true; break;
                case ITEM_AMMO_MINIGUN:   player_add_ammo(player,WPN_MINIGUN, 150); picked=true; break;
                case ITEM_AMMO_CRYO:      player_add_ammo(player,WPN_CRYO,     25); picked=true; break;
                case ITEM_AMMO_FLAME:     player_add_ammo(player,WPN_FLAMETHROWER,50); picked=true; break;
                case ITEM_WEAPON: {
                    WeaponType wt = e.weaponType;
                    int wi = (int)wt;
                    if (!player.weapons[wi].owned) {
                        player.weapons[wi].owned = true;
                        player.weapons[wi].ammo  = weapon_ammo_max[wi] / 4;
                        picked = true;
                    } else if (action_use()) {
                        int old = player.currentWeapon;
                        player.weapons[wi].owned = true;
                        player.weapons[wi].ammo  = weapon_ammo_max[wi] / 4;
                        player.weapons[old].owned= false;
                        e.weaponType = (WeaponType)old;
                    }
                    break;
                }
                default: break;
                }
                if (picked) e.active = false;
            }
        }
    }
    return playerHit;
}

inline void entities_spawn_for_level(int level, uint32_t seed) {
    entities_clear();
    g_ent_rng = seed ^ 0xABCDEF12u;
    double t  = (double)level / (double)MAX_LEVELS;

    for (int r=0; r<g_world.roomCount; r++) {
        if (r == 0) continue;
        Room& rm = g_world.rooms[r];

        int enemyCount = ent_rng_range(1, (int)(2 + t*5));
        enemyCount = std::min(enemyCount, 8);
        for (int ei=0; ei<enemyCount; ei++) {
            int maxType = std::min(14, (int)(t*15));
            EnemyType et = (EnemyType)ent_rng_range(0, std::max(1,maxType+1));
            if (level % 10 == 0 && ei == 0) et = ENEMY_BOSS;
            double ex = rm.x + 1 + ent_rng_range(0, rm.w-2);
            double ey = rm.y + 1 + ent_rng_range(0, rm.h-2);
            entity_spawn_enemy(et, {ex+0.5, ey+0.5});
        }

        int itemCount = ent_rng_range(1, 4);
        for (int ii=0; ii<itemCount; ii++) {
            double ix = rm.x + 1 + ent_rng_range(0, rm.w-2);
            double iy = rm.y + 1 + ent_rng_range(0, rm.h-2);
            double roll = ent_rng_float();
            ItemType it;
            if      (roll < 0.30) it = ITEM_HEALTH_SMALL;
            else if (roll < 0.42) it = ITEM_HEALTH_BIG;
            else if (roll < 0.52) it = ITEM_ARMOR;
            else if (roll < 0.65) it = ITEM_AMMO_PISTOL;
            else if (roll < 0.74) it = ITEM_AMMO_SHOTGUN;
            else if (roll < 0.83) it = ITEM_AMMO_AUTO;
            else                  it = ITEM_AMMO_ROCKET;
            entity_spawn_item(it, {ix+0.5, iy+0.5});
        }

        double weaponRoll = ent_rng_float();
        if (weaponRoll < 0.15 + t*0.2) {
            double wx = rm.x + 1 + ent_rng_range(0, rm.w-2);
            double wy = rm.y + 1 + ent_rng_range(0, rm.h-2);
            int maxW  = std::min(14, 1 + (int)(t*13));
            WeaponType wt = (WeaponType)ent_rng_range(1, maxW+1);
            entity_spawn_item(ITEM_WEAPON, {wx+0.5, wy+0.5}, wt);
        }
    }
}

inline int entities_count_alive() {
    int n=0;
    for (int i=0;i<g_entityCount;i++)
        if (g_entities[i].active&&g_entities[i].type==ENT_ENEMY&&g_entities[i].state!=ES_DEAD)
            n++;
    return n;
}

inline int entities_count_frozen() {
    int n=0;
    for (int i=0;i<g_entityCount;i++)
        if (g_entities[i].active&&g_entities[i].frozen) n++;
    return n;
}

inline int entities_count_aggro() {
    int n=0;
    for (int i=0;i<g_entityCount;i++)
        if (g_entities[i].active&&g_entities[i].inAggro) n++;
    return n;
}
