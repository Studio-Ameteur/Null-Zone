#pragma once
#include <SDL.h>
#include <SDL_mixer.h>
#include "common.h"
#include "player.h"

static const int AUDIO_FREQ     = 22050;
static const int AUDIO_CHANNELS = 16;
static const int AUDIO_CHUNK    = 512;

enum SoundID {
    SND_PISTOL,
    SND_SHOTGUN,
    SND_AUTO,
    SND_SNIPER,
    SND_ROCKET,
    SND_PLASMA,
    SND_SUPER_SHOTGUN,
    SND_FLAMETHROWER,
    SND_RAILGUN,
    SND_GRENADE,
    SND_MINIGUN,
    SND_CRYO,
    SND_CHAINSAW,
    SND_BFG,
    SND_FISTS,
    SND_STEP_METAL,
    SND_STEP_CONCRETE,
    SND_ENEMY_ALERT,
    SND_ENEMY_ATTACK,
    SND_ENEMY_DEATH,
    SND_PICKUP_HEALTH,
    SND_PICKUP_ARMOR,
    SND_PICKUP_AMMO,
    SND_PICKUP_WEAPON,
    SND_DOOR_OPEN,
    SND_PLAYER_HURT,
    SND_PLAYER_DEATH,
    SND_LEVEL_COMPLETE,
    SND_COUNT
};

static Mix_Chunk* g_sounds[SND_COUNT] = {};
static bool       g_audio_ok          = false;
static double     g_music_timer       = 0.0;
static int        g_music_channel     = -1;

static Mix_Chunk* make_chunk(const Sint16* samples, int count) {
    int bytes = count * sizeof(Sint16);
    Uint8* buf = (Uint8*)SDL_malloc(bytes);
    if (!buf) return nullptr;
    SDL_memcpy(buf, samples, bytes);
    Mix_Chunk* c = (Mix_Chunk*)SDL_malloc(sizeof(Mix_Chunk));
    if (!c) { SDL_free(buf); return nullptr; }
    c->allocated = 1;
    c->abuf      = buf;
    c->alen      = (Uint32)bytes;
    c->volume    = MIX_MAX_VOLUME;
    return c;
}

static Mix_Chunk* gen_shoot_pistol() {
    const int N = AUDIO_FREQ / 8;
    static Sint16 s[AUDIO_FREQ/8];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*40.0);
        double v  = sin(2*M_PI*800*t) * env * 0.6
                  + sin(2*M_PI*200*t) * env * 0.4
                  + ((rand()%32768-16384)/32768.0) * env * 0.3;
        s[i] = (Sint16)(v * 28000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_shotgun() {
    const int N = AUDIO_FREQ / 5;
    static Sint16 s[AUDIO_FREQ/5];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*18.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise * env * 0.9
                  + sin(2*M_PI*120*t) * env * 0.4;
        s[i] = (Sint16)(v * 30000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_auto() {
    const int N = AUDIO_FREQ / 12;
    static Sint16 s[AUDIO_FREQ/12];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*60.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise * env * 0.7
                  + sin(2*M_PI*400*t) * env * 0.3;
        s[i] = (Sint16)(v * 25000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_sniper() {
    const int N = AUDIO_FREQ / 6;
    static Sint16 s[AUDIO_FREQ/6];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*12.0);
        double v  = sin(2*M_PI*1200*t*exp(-t*8)) * env * 0.5
                  + ((rand()%32768-16384)/32768.0) * env * 0.5;
        s[i] = (Sint16)(v * 29000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_rocket() {
    const int N = AUDIO_FREQ / 4;
    static Sint16 s[AUDIO_FREQ/4];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*8.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = sin(2*M_PI*80*t) * env * 0.5
                  + noise * env * 0.6
                  + sin(2*M_PI*40*t) * env * 0.3;
        s[i] = (Sint16)(v * 28000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_plasma() {
    const int N = AUDIO_FREQ / 6;
    static Sint16 s[AUDIO_FREQ/6];
    for (int i=0;i<N;i++) {
        double t   = (double)i/AUDIO_FREQ;
        double env = exp(-t*20.0);
        double v   = sin(2*M_PI*(600-t*300)*t) * env * 0.7
                   + sin(2*M_PI*1800*t) * env * 0.2;
        s[i] = (Sint16)(v * 26000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_super_shotgun() {
    const int N = AUDIO_FREQ / 4;
    static Sint16 s[AUDIO_FREQ/4];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*12.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise * env * 1.0
                  + sin(2*M_PI*80*t) * env * 0.5;
        s[i] = (Sint16)(std::min(32767.0, v * 32000));
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_flamethrower() {
    const int N = AUDIO_FREQ / 10;
    static Sint16 s[AUDIO_FREQ/10];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= 0.8 - t*2.0; if(env<0)env=0;
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise * env * 0.9
                  + sin(2*M_PI*200*t) * env * 0.1;
        s[i] = (Sint16)(v * 20000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_railgun() {
    const int N = AUDIO_FREQ / 5;
    static Sint16 s[AUDIO_FREQ/5];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*10.0);
        double v  = sin(2*M_PI*(2000-t*1500)*t) * env * 0.8;
        s[i] = (Sint16)(v * 30000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_grenade() {
    const int N = AUDIO_FREQ / 6;
    static Sint16 s[AUDIO_FREQ/6];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*15.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = sin(2*M_PI*150*t) * env * 0.5
                  + noise * env * 0.5;
        s[i] = (Sint16)(v * 27000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_minigun() {
    const int N = AUDIO_FREQ / 15;
    static Sint16 s[AUDIO_FREQ/15];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*80.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise * env * 0.8
                  + sin(2*M_PI*300*t) * env * 0.2;
        s[i] = (Sint16)(v * 22000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_shoot_cryo() {
    const int N = AUDIO_FREQ / 7;
    static Sint16 s[AUDIO_FREQ/7];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*14.0);
        double v  = sin(2*M_PI*(1200+sin(t*30)*200)*t) * env * 0.6
                  + ((rand()%32768-16384)/32768.0) * env * 0.2;
        s[i] = (Sint16)(v * 24000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_chainsaw() {
    const int N = AUDIO_FREQ / 8;
    static Sint16 s[AUDIO_FREQ/8];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double saw= fmod(t*120.0, 1.0)*2.0-1.0;
        double noise = ((rand()%32768-16384)/32768.0)*0.3;
        s[i] = (Sint16)((saw*0.7+noise) * 22000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_bfg() {
    const int N = AUDIO_FREQ / 2;
    static Sint16 s[AUDIO_FREQ/2];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*4.0);
        double v  = sin(2*M_PI*(300-t*200)*t) * env * 0.6
                  + sin(2*M_PI*60*t) * env * 0.4
                  + ((rand()%32768-16384)/32768.0) * env * 0.3;
        s[i] = (Sint16)(std::min(32767.0, v * 32000));
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_fists() {
    const int N = AUDIO_FREQ / 12;
    static Sint16 s[AUDIO_FREQ/12];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*50.0);
        double noise = ((rand()%32768-16384)/32768.0);
        s[i] = (Sint16)(noise * env * 20000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_step_metal() {
    const int N = AUDIO_FREQ / 15;
    static Sint16 s[AUDIO_FREQ/15];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*60.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise*env*0.5 + sin(2*M_PI*800*t)*env*0.3;
        s[i] = (Sint16)(v * 12000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_step_concrete() {
    const int N = AUDIO_FREQ / 15;
    static Sint16 s[AUDIO_FREQ/15];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*50.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise*env*0.6 + sin(2*M_PI*300*t)*env*0.2;
        s[i] = (Sint16)(v * 10000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_enemy_alert() {
    const int N = AUDIO_FREQ / 5;
    static Sint16 s[AUDIO_FREQ/5];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*8.0);
        double v  = sin(2*M_PI*(400+t*200)*t)*env*0.7
                  + sin(2*M_PI*800*t)*env*0.2;
        s[i] = (Sint16)(v * 18000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_enemy_attack() {
    const int N = AUDIO_FREQ / 8;
    static Sint16 s[AUDIO_FREQ/8];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*25.0);
        double noise = ((rand()%32768-16384)/32768.0);
        s[i] = (Sint16)(noise * env * 16000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_enemy_death() {
    const int N = AUDIO_FREQ / 4;
    static Sint16 s[AUDIO_FREQ/4];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*10.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise*env*0.6 + sin(2*M_PI*(200-t*150)*t)*env*0.4;
        s[i] = (Sint16)(v * 20000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_pickup_health() {
    const int N = AUDIO_FREQ / 8;
    static Sint16 s[AUDIO_FREQ/8];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*15.0);
        double v  = sin(2*M_PI*880*t)*env*0.5
                  + sin(2*M_PI*1320*t)*env*0.3;
        s[i] = (Sint16)(v * 18000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_pickup_armor() {
    const int N = AUDIO_FREQ / 8;
    static Sint16 s[AUDIO_FREQ/8];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*12.0);
        double v  = sin(2*M_PI*660*t)*env*0.4
                  + sin(2*M_PI*990*t)*env*0.4;
        s[i] = (Sint16)(v * 16000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_pickup_ammo() {
    const int N = AUDIO_FREQ / 10;
    static Sint16 s[AUDIO_FREQ/10];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*20.0);
        double v  = sin(2*M_PI*440*t)*env*0.4
                  + sin(2*M_PI*550*t)*env*0.3;
        s[i] = (Sint16)(v * 14000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_pickup_weapon() {
    const int N = AUDIO_FREQ / 6;
    static Sint16 s[AUDIO_FREQ/6];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*10.0);
        double v  = sin(2*M_PI*523*t)*env*0.3
                  + sin(2*M_PI*659*t)*env*0.3
                  + sin(2*M_PI*784*t)*env*0.3;
        s[i] = (Sint16)(v * 18000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_door_open() {
    const int N = AUDIO_FREQ / 5;
    static Sint16 s[AUDIO_FREQ/5];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*6.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise*env*0.4 + sin(2*M_PI*150*t)*env*0.4;
        s[i] = (Sint16)(v * 14000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_player_hurt() {
    const int N = AUDIO_FREQ / 6;
    static Sint16 s[AUDIO_FREQ/6];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*12.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise*env*0.5 + sin(2*M_PI*(300-t*100)*t)*env*0.4;
        s[i] = (Sint16)(v * 22000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_player_death() {
    const int N = AUDIO_FREQ / 2;
    static Sint16 s[AUDIO_FREQ/2];
    for (int i=0;i<N;i++) {
        double t  = (double)i/AUDIO_FREQ;
        double env= exp(-t*3.0);
        double noise = ((rand()%32768-16384)/32768.0);
        double v  = noise*env*0.4 + sin(2*M_PI*(200-t*150)*t)*env*0.5;
        s[i] = (Sint16)(v * 25000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_level_complete() {
    const int N = AUDIO_FREQ / 2;
    static Sint16 s[AUDIO_FREQ/2];
    static const double notes[] = {523,659,784,1047};
    for (int i=0;i<N;i++) {
        double t   = (double)i/AUDIO_FREQ;
        int ni     = (int)(t*8) % 4;
        double env = exp(-fmod(t*8,1.0)*5.0);
        double v   = sin(2*M_PI*notes[ni]*t)*env*0.6;
        s[i] = (Sint16)(v * 22000);
    }
    return make_chunk(s, N);
}

static Mix_Chunk* gen_ambient_music(int level) {
    const int N = AUDIO_FREQ * 4;
    Sint16* s = (Sint16*)SDL_malloc(N * sizeof(Sint16));
    if (!s) return nullptr;
    double t_level = (double)level / MAX_LEVELS;
    double baseFreq = 55.0 + t_level * 30.0;
    double tension  = 0.3 + t_level * 0.7;
    for (int i=0;i<N;i++) {
        double t = (double)i/AUDIO_FREQ;
        double v = sin(2*M_PI*baseFreq*t) * 0.25
                 + sin(2*M_PI*baseFreq*1.5*t) * 0.15 * tension
                 + sin(2*M_PI*baseFreq*2.0*t) * 0.10
                 + sin(2*M_PI*baseFreq*0.5*t) * 0.20
                 + sin(2*M_PI*(baseFreq*3.0+sin(t*0.3)*20)*t) * 0.08 * tension
                 + ((rand()%32768-16384)/32768.0) * 0.02;
        double pulse = 0.7 + 0.3*sin(2*M_PI*0.25*t);
        s[i] = (Sint16)(v * pulse * 8000);
    }
    Mix_Chunk* c = (Mix_Chunk*)SDL_malloc(sizeof(Mix_Chunk));
    if (!c) { SDL_free(s); return nullptr; }
    c->allocated = 1;
    c->abuf      = (Uint8*)s;
    c->alen      = (Uint32)(N * sizeof(Sint16));
    c->volume    = MIX_MAX_VOLUME / 3;
    return c;
}

static Mix_Chunk* g_music_chunk = nullptr;

inline bool audio_init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) return false;
    if (Mix_OpenAudio(AUDIO_FREQ, AUDIO_S16SYS, 1, AUDIO_CHUNK) < 0) return false;
    Mix_AllocateChannels(AUDIO_CHANNELS);
    g_audio_ok = true;

    g_sounds[SND_PISTOL]        = gen_shoot_pistol();
    g_sounds[SND_SHOTGUN]       = gen_shoot_shotgun();
    g_sounds[SND_AUTO]          = gen_shoot_auto();
    g_sounds[SND_SNIPER]        = gen_shoot_sniper();
    g_sounds[SND_ROCKET]        = gen_shoot_rocket();
    g_sounds[SND_PLASMA]        = gen_shoot_plasma();
    g_sounds[SND_SUPER_SHOTGUN] = gen_shoot_super_shotgun();
    g_sounds[SND_FLAMETHROWER]  = gen_shoot_flamethrower();
    g_sounds[SND_RAILGUN]       = gen_shoot_railgun();
    g_sounds[SND_GRENADE]       = gen_shoot_grenade();
    g_sounds[SND_MINIGUN]       = gen_shoot_minigun();
    g_sounds[SND_CRYO]          = gen_shoot_cryo();
    g_sounds[SND_CHAINSAW]      = gen_chainsaw();
    g_sounds[SND_BFG]           = gen_bfg();
    g_sounds[SND_FISTS]         = gen_fists();
    g_sounds[SND_STEP_METAL]    = gen_step_metal();
    g_sounds[SND_STEP_CONCRETE] = gen_step_concrete();
    g_sounds[SND_ENEMY_ALERT]   = gen_enemy_alert();
    g_sounds[SND_ENEMY_ATTACK]  = gen_enemy_attack();
    g_sounds[SND_ENEMY_DEATH]   = gen_enemy_death();
    g_sounds[SND_PICKUP_HEALTH] = gen_pickup_health();
    g_sounds[SND_PICKUP_ARMOR]  = gen_pickup_armor();
    g_sounds[SND_PICKUP_AMMO]   = gen_pickup_ammo();
    g_sounds[SND_PICKUP_WEAPON] = gen_pickup_weapon();
    g_sounds[SND_DOOR_OPEN]     = gen_door_open();
    g_sounds[SND_PLAYER_HURT]   = gen_player_hurt();
    g_sounds[SND_PLAYER_DEATH]  = gen_player_death();
    g_sounds[SND_LEVEL_COMPLETE]= gen_level_complete();

    return true;
}

inline void audio_play_music(int level) {
    if (!g_audio_ok) return;
    if (g_music_chunk) {
        if (g_music_channel >= 0) Mix_HaltChannel(g_music_channel);
        Mix_FreeChunk(g_music_chunk);
        g_music_chunk = nullptr;
    }
    g_music_chunk   = gen_ambient_music(level);
    if (g_music_chunk)
        g_music_channel = Mix_PlayChannel(-1, g_music_chunk, -1);
}

inline void audio_play(SoundID id, double vol=1.0) {
    if (!g_audio_ok || id >= SND_COUNT) return;
    Mix_Chunk* c = g_sounds[id];
    if (!c) return;
    int ch = Mix_PlayChannel(-1, c, 0);
    if (ch >= 0) Mix_Volume(ch, (int)(vol * MIX_MAX_VOLUME));
}

inline void audio_play_3d(SoundID id, Vec2 soundPos, Vec2 playerPos, double maxDist=15.0) {
    if (!g_audio_ok) return;
    double dist = soundPos.distTo(playerPos);
    if (dist > maxDist) return;
    double vol = 1.0 - (dist / maxDist);
    vol = vol * vol;
    audio_play(id, vol);
}

inline SoundID weapon_sound(int weaponIdx) {
    static const SoundID map[MAX_WEAPONS] = {
        SND_FISTS, SND_PISTOL, SND_SHOTGUN, SND_AUTO, SND_SNIPER,
        SND_ROCKET, SND_PLASMA, SND_SUPER_SHOTGUN, SND_FLAMETHROWER,
        SND_RAILGUN, SND_GRENADE, SND_MINIGUN, SND_CRYO, SND_CHAINSAW, SND_BFG
    };
    if (weaponIdx < 0 || weaponIdx >= MAX_WEAPONS) return SND_PISTOL;
    return map[weaponIdx];
}

inline SoundID step_sound(Vec2 pos) {
    int tx = (int)pos.x, ty = (int)pos.y;
    if (!world_in_bounds(tx, ty)) return SND_STEP_METAL;
    int ft = g_world.map[ty][tx].texFloor;
    return (ft == 17 || ft == 18) ? SND_STEP_CONCRETE : SND_STEP_METAL;
}

inline void audio_shutdown() {
    if (!g_audio_ok) return;
    Mix_HaltChannel(-1);
    if (g_music_chunk) { Mix_FreeChunk(g_music_chunk); g_music_chunk=nullptr; }
    for (int i=0;i<SND_COUNT;i++) {
        if (g_sounds[i]) { Mix_FreeChunk(g_sounds[i]); g_sounds[i]=nullptr; }
    }
    Mix_CloseAudio();
    g_audio_ok = false;
}
