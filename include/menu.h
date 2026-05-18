#pragma once
#include <SDL.h>
#include "common.h"
#include "hud.h"
#include "savegame.h"
#include "input.h"

enum MenuID {
    MENU_MAIN, MENU_PAUSE, MENU_SAVE, MENU_LOAD,
    MENU_SETTINGS, MENU_CONFIRM_DELETE, MENU_ERROR
};

struct MenuState {
    MenuID  current;
    int     selectedItem;
    int     selectedSlot;
    bool    deleteConfirm;
    char    errorMsg[256];
    double  animTimer;
    GameState returnState;
};

static MenuState g_menu = {};
static int g_mouse_sens = 2;
static int g_sound_vol  = 100;
static int g_music_vol  = 100;

static const char* main_items[]  = {"NEW GAME","CONTINUE","LOAD GAME","SETTINGS","QUIT"};
static const int   MAIN_COUNT    = 5;
static const char* pause_items[] = {"RESUME","SAVE GAME","LOAD GAME","MAIN MENU","QUIT TO DESKTOP"};
static const int   PAUSE_COUNT   = 5;

inline void menu_init() {
    memset(&g_menu,0,sizeof(g_menu));
    g_menu.current     = MENU_MAIN;
    g_menu.selectedSlot= 1;
    g_menu.returnState = GS_MENU;
}

static int _sw=SCREEN_W, _sh=SCREEN_H;

static void menu_sync_size(SDL_Renderer* r) {
    SDL_GetRendererOutputSize(r,&_sw,&_sh);
}

static void m_rect(SDL_Renderer* r,int x,int y,int w,int h,const Color& c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_SetRenderDrawBlendMode(r,SDL_BLENDMODE_BLEND);
    SDL_Rect rc={x,y,w,h}; SDL_RenderFillRect(r,&rc);
}

static void m_rect_out(SDL_Renderer* r,int x,int y,int w,int h,const Color& c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_Rect rc={x,y,w,h}; SDL_RenderDrawRect(r,&rc);
}

static void m_text_c(SDL_Renderer* r,TTF_Font* f,const char* t,int cx,int y,const Color& c) {
    if(!f||!t) return;
    int w,h; TTF_SizeText(f,t,&w,&h);
    SDL_Color sc={c.r,c.g,c.b,c.a};
    SDL_Surface* surf=TTF_RenderText_Blended(f,t,sc);
    if(!surf) return;
    SDL_Texture* tex=SDL_CreateTextureFromSurface(r,surf);
    if(tex){SDL_Rect dst={cx-w/2,y,surf->w,surf->h};SDL_RenderCopy(r,tex,nullptr,&dst);SDL_DestroyTexture(tex);}
    SDL_FreeSurface(surf);
}

static void m_text(SDL_Renderer* r,TTF_Font* f,const char* t,int x,int y,const Color& c) {
    if(!f||!t) return;
    SDL_Color sc={c.r,c.g,c.b,c.a};
    SDL_Surface* surf=TTF_RenderText_Blended(f,t,sc);
    if(!surf) return;
    SDL_Texture* tex=SDL_CreateTextureFromSurface(r,surf);
    if(tex){SDL_Rect dst={x,y,surf->w,surf->h};SDL_RenderCopy(r,tex,nullptr,&dst);SDL_DestroyTexture(tex);}
    SDL_FreeSurface(surf);
}

static void draw_stars(SDL_Renderer* r, double anim) {
    m_rect(r,0,0,_sw,_sh,HUD_DARK);
    for(int i=0;i<100;i++){
        uint32_t rx=(uint32_t)(i*73856931u)^(uint32_t)(anim*2+i)*1234567u;
        rx^=rx<<13; rx^=rx>>17; rx^=rx<<5;
        int sx=(int)(rx%_sw), sy=(int)((rx>>12)%_sh);
        double bri=((rx>>20)%80+10)/255.0;
        double flicker=0.7+0.3*sin(anim*2.3+i*0.7);
        uint8_t b=(uint8_t)(bri*flicker*255);
        SDL_SetRenderDrawColor(r,b,b,(uint8_t)std::min(255,b+20),255);
        SDL_RenderDrawPoint(r,sx,sy);
    }
}

static void draw_logo(SDL_Renderer* r, double anim) {
    int cx=_sw/2;
    int ty=_sh/10;
    int bw=_sw/2, bh=_sh/8;
    int bx=cx-bw/2;

    m_rect(r,bx,ty,bw,bh,{5,3,10,210});
    m_rect_out(r,bx,ty,bw,bh,HUD_ACCENT);

    SDL_SetRenderDrawColor(r,HUD_ACCENT.r,HUD_ACCENT.g,HUD_ACCENT.b,180);
    SDL_RenderDrawLine(r,bx+8,ty+bh-2,bx+bw-8,ty+bh-2);

    Color c1={(uint8_t)(200+sin(anim)*20),(uint8_t)(140+sin(anim*1.2)*15),20,255};
    m_text_c(r,g_font_large,"NULL ZONE",cx,ty+bh/8,c1);

    char sub[64]; SDL_snprintf(sub,sizeof(sub),"v%s  by %s",NZ_VERSION,NZ_AUTHOR);
    m_text_c(r,g_font_small,sub,cx,ty+bh*2/3,HUD_GRAY);
}

static void draw_items(SDL_Renderer* r,const char** items,int count,int sel,int startY) {
    int cx=_sw/2;
    int ih=_sh/12;
    for(int i=0;i<count;i++){
        int iy=startY+i*ih;
        bool s=(i==sel);
        int bw=_sw/3, bh=ih-4;
        if(s){
            m_rect(r,cx-bw/2,iy-2,bw,bh,{40,25,5,200});
            m_rect_out(r,cx-bw/2,iy-2,bw,bh,HUD_ACCENT);
            m_text_c(r,g_font_small,">",cx-bw/2+8,iy,HUD_ORANGE);
            m_text_c(r,g_font_small,"<",cx+bw/2-8,iy,HUD_ORANGE);
        }
        Color tc=s?HUD_ACCENT:HUD_WHITE;
        m_text_c(r,g_font_medium,items[i],cx,iy,tc);
    }
}

static void draw_save_load(SDL_Renderer* r,bool isSave,int selSlot,double anim) {
    int bw=_sw*3/5, bh=_sh*3/4;
    int bx=_sw/2-bw/2, by=_sh/8;
    m_rect(r,bx,by,bw,bh,{5,3,10,235});
    m_rect_out(r,bx,by,bw,bh,HUD_ACCENT);

    m_text_c(r,g_font_large,isSave?"[ SAVE GAME ]":"[ LOAD GAME ]",_sw/2,by+8,HUD_ACCENT);

    int rowH=bh/11;
    int ay=by+rowH;
    bool asel=(selSlot==0);
    if(asel) m_rect(r,bx+4,ay,bw-8,rowH-2,{40,25,5,180});
    char autobuf[128]; savegame_get_slot_info(0,autobuf,sizeof(autobuf));
    m_text(r,g_font_small,"AUTO",bx+10,ay+4,HUD_GRAY);
    m_text(r,g_font_small,autobuf,bx+80,ay+4,asel?HUD_ACCENT:HUD_GRAY);

    SDL_SetRenderDrawColor(r,HUD_GRAY.r,HUD_GRAY.g,HUD_GRAY.b,80);
    SDL_RenderDrawLine(r,bx+8,by+rowH*2-2,bx+bw-8,by+rowH*2-2);

    for(int i=1;i<=MAX_SAVE_SLOTS;i++){
        int sy=by+rowH*2+(i-1)*rowH;
        bool sel=(i==selSlot);
        if(sel){m_rect(r,bx+4,sy,bw-8,rowH-2,{40,25,5,200});m_rect_out(r,bx+4,sy,bw-8,rowH-2,HUD_ACCENT);}
        char lbl[16]; SDL_snprintf(lbl,sizeof(lbl),"SLOT %d",i);
        m_text(r,g_font_small,lbl,bx+10,sy+4,sel?HUD_ACCENT:HUD_GRAY);
        char info[128]; savegame_get_slot_info((uint8_t)i,info,sizeof(info));
        m_text(r,g_font_small,info,bx+80,sy+4,sel?(savegame_slot_exists((uint8_t)i)?HUD_WHITE:HUD_GRAY):HUD_GRAY);
        if(sel&&savegame_slot_exists((uint8_t)i))
            m_text(r,g_font_small,"[DEL]",bx+bw-60,sy+4,HUD_RED);
    }
    double pulse=0.6+0.4*sin(anim*3.0);
    Color pc={(uint8_t)(200*pulse),(uint8_t)(140*pulse),20,255};
    m_text_c(r,g_font_small,"UP/DOWN: Select   ENTER: Confirm   DEL: Delete   ESC: Back",_sw/2,by+bh-22,pc);
}

static void draw_confirm_delete(SDL_Renderer* r,int slot) {
    int bw=_sw/3, bh=_sh/7;
    int bx=_sw/2-bw/2, by=_sh/2-bh/2;
    m_rect(r,bx,by,bw,bh,{20,4,4,245});
    m_rect_out(r,bx,by,bw,bh,HUD_RED);
    char buf[64]; SDL_snprintf(buf,sizeof(buf),"Delete slot %d?",slot);
    m_text_c(r,g_font_medium,buf,_sw/2,by+bh/5,HUD_RED);
    m_text_c(r,g_font_small,"ENTER: Yes   ESC: No",_sw/2,by+bh*3/5,HUD_WHITE);
}

static void draw_error(SDL_Renderer* r,const char* msg) {
    int bw=_sw/2, bh=_sh/6;
    int bx=_sw/2-bw/2, by=_sh/2-bh/2;
    m_rect(r,bx,by,bw,bh,{20,4,4,245});
    m_rect_out(r,bx,by,bw,bh,HUD_RED);
    m_text_c(r,g_font_medium,"ERROR",_sw/2,by+bh/6,HUD_RED);
    m_text_c(r,g_font_small,msg,_sw/2,by+bh/2,HUD_WHITE);
    m_text_c(r,g_font_small,"Press any key",_sw/2,by+bh*3/4,HUD_GRAY);
}

static void draw_settings(SDL_Renderer* r,int sel) {
    static const char* sens_names[]={"LOW","MEDIUM","HIGH","VERY HIGH"};
    char items[5][64];
    SDL_snprintf(items[0],64,"MOUSE SENSITIVITY: %s",sens_names[std::min(g_mouse_sens,3)]);
    SDL_snprintf(items[1],64,"SOUND VOLUME: %d%%",g_sound_vol);
    SDL_snprintf(items[2],64,"MUSIC VOLUME: %d%%",g_music_vol);
    SDL_snprintf(items[3],64,"RESOLUTION: %dx%d",_sw,_sh);
    SDL_snprintf(items[4],64,"BACK");
    const char* ptrs[5]; for(int i=0;i<5;i++) ptrs[i]=items[i];
    draw_items(r,ptrs,5,sel,_sh/3);
}

inline void menu_render(SDL_Renderer* r, double dt) {
    menu_sync_size(r);
    g_menu.animTimer+=dt;
    double anim=g_menu.animTimer;

    draw_stars(r,anim);

    switch(g_menu.current){
    case MENU_MAIN:
        draw_logo(r,anim);
        draw_items(r,main_items,MAIN_COUNT,g_menu.selectedItem,_sh/3);
        m_text_c(r,g_font_small,NZ_AUTHOR "  |  " NZ_EMAIL "  |  " NZ_YEAR,_sw/2,_sh-20,HUD_GRAY);
        break;
    case MENU_PAUSE:
        m_rect(r,_sw/2-_sw/4,_sh/8,_sw/2,_sh/8,{5,3,10,220});
        m_rect_out(r,_sw/2-_sw/4,_sh/8,_sw/2,_sh/8,HUD_ACCENT);
        m_text_c(r,g_font_large,"PAUSED",_sw/2,_sh/8+8,HUD_ACCENT);
        draw_items(r,pause_items,PAUSE_COUNT,g_menu.selectedItem,_sh/3);
        break;
    case MENU_SAVE:
        draw_save_load(r,true,g_menu.selectedSlot,anim);
        if(g_menu.deleteConfirm) draw_confirm_delete(r,g_menu.selectedSlot);
        break;
    case MENU_LOAD:
        draw_save_load(r,false,g_menu.selectedSlot,anim);
        if(g_menu.deleteConfirm) draw_confirm_delete(r,g_menu.selectedSlot);
        break;
    case MENU_SETTINGS:
        draw_logo(r,anim);
        m_text_c(r,g_font_large,"SETTINGS",_sw/2,_sh/4,HUD_ACCENT);
        draw_settings(r,g_menu.selectedItem);
        break;
    case MENU_ERROR:
        draw_error(r,g_menu.errorMsg);
        break;
    default: break;
    }
}

enum MenuAction {
    MACT_NONE, MACT_NEW_GAME, MACT_CONTINUE, MACT_RESUME,
    MACT_LOAD_OK, MACT_GOTO_MAIN, MACT_QUIT
};

inline MenuAction menu_update(Player& p, GameState& gs) {
    bool up   =key_pressed(SDL_SCANCODE_UP)  ||key_pressed(SDL_SCANCODE_W);
    bool down =key_pressed(SDL_SCANCODE_DOWN)||key_pressed(SDL_SCANCODE_S);
    bool enter=key_pressed(SDL_SCANCODE_RETURN)||key_pressed(SDL_SCANCODE_KP_ENTER);
    bool esc  =action_pause();
    bool del  =key_pressed(SDL_SCANCODE_DELETE);
    bool left =key_pressed(SDL_SCANCODE_LEFT)||key_pressed(SDL_SCANCODE_A);
    bool right=key_pressed(SDL_SCANCODE_RIGHT)||key_pressed(SDL_SCANCODE_D);

    if(g_menu.current==MENU_ERROR){
        if(enter||esc||del||up||down)
            g_menu.current=(g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        return MACT_NONE;
    }

    if(g_menu.deleteConfirm){
        if(enter){savegame_delete((uint8_t)g_menu.selectedSlot);g_menu.deleteConfirm=false;}
        else if(esc) g_menu.deleteConfirm=false;
        return MACT_NONE;
    }

    switch(g_menu.current){
    case MENU_MAIN:
        if(up)   g_menu.selectedItem=(g_menu.selectedItem-1+MAIN_COUNT)%MAIN_COUNT;
        if(down) g_menu.selectedItem=(g_menu.selectedItem+1)%MAIN_COUNT;
        if(enter){
            switch(g_menu.selectedItem){
            case 0: return MACT_NEW_GAME;
            case 1:
                if(savegame_slot_exists(0)||savegame_slot_exists(1)){
                    SaveLoadResult res=savegame_read(0,p);
                    if(res!=SAVE_OK) res=savegame_read(1,p);
                    if(res==SAVE_OK){gs=GS_PLAYING;return MACT_CONTINUE;}
                }
                SDL_strlcpy(g_menu.errorMsg,"No save file found. Start a new game.",sizeof(g_menu.errorMsg));
                g_menu.returnState=GS_MENU; g_menu.current=MENU_ERROR;
                break;
            case 2: g_menu.current=MENU_LOAD; g_menu.selectedSlot=1; break;
            case 3: g_menu.current=MENU_SETTINGS; g_menu.selectedItem=0; break;
            case 4: return MACT_QUIT;
            }
        }
        break;
    case MENU_PAUSE:
        if(up)   g_menu.selectedItem=(g_menu.selectedItem-1+PAUSE_COUNT)%PAUSE_COUNT;
        if(down) g_menu.selectedItem=(g_menu.selectedItem+1)%PAUSE_COUNT;
        if(esc)  return MACT_RESUME;
        if(enter){
            switch(g_menu.selectedItem){
            case 0: return MACT_RESUME;
            case 1: g_menu.current=MENU_SAVE; g_menu.selectedSlot=1; break;
            case 2: g_menu.current=MENU_LOAD; g_menu.selectedSlot=1; break;
            case 3: gs=GS_MENU; g_menu.current=MENU_MAIN; g_menu.selectedItem=0; return MACT_GOTO_MAIN;
            case 4: return MACT_QUIT;
            }
        }
        break;
    case MENU_SAVE:
        if(up)   g_menu.selectedSlot=std::max(0,g_menu.selectedSlot-1);
        if(down) g_menu.selectedSlot=std::min(MAX_SAVE_SLOTS,g_menu.selectedSlot+1);
        if(del&&g_menu.selectedSlot>0&&savegame_slot_exists((uint8_t)g_menu.selectedSlot))
            g_menu.deleteConfirm=true;
        if(enter&&g_menu.selectedSlot>0){savegame_write(p,(uint8_t)g_menu.selectedSlot);g_menu.current=MENU_PAUSE;}
        if(esc) g_menu.current=MENU_PAUSE;
        break;
    case MENU_LOAD:
        if(up)   g_menu.selectedSlot=std::max(0,g_menu.selectedSlot-1);
        if(down) g_menu.selectedSlot=std::min(MAX_SAVE_SLOTS,g_menu.selectedSlot+1);
        if(del&&g_menu.selectedSlot>0&&savegame_slot_exists((uint8_t)g_menu.selectedSlot))
            g_menu.deleteConfirm=true;
        if(enter){
            SaveLoadResult res=savegame_read((uint8_t)g_menu.selectedSlot,p);
            if(res==SAVE_OK){gs=GS_PLAYING;return MACT_LOAD_OK;}
            SDL_snprintf(g_menu.errorMsg,sizeof(g_menu.errorMsg),"Failed: %s",savegame_error_str(res));
            g_menu.returnState=gs; g_menu.current=MENU_ERROR;
        }
        if(esc) g_menu.current=(g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        break;
    case MENU_SETTINGS:
        if(up)   g_menu.selectedItem=(g_menu.selectedItem-1+5)%5;
        if(down) g_menu.selectedItem=(g_menu.selectedItem+1)%5;
        if((enter||esc)&&g_menu.selectedItem==4)
            g_menu.current=(g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        if(esc&&g_menu.selectedItem!=4)
            g_menu.current=(g_menu.returnState==GS_PLAYING)?MENU_PAUSE:MENU_MAIN;
        if(left||right){
            int d=right?1:-1;
            switch(g_menu.selectedItem){
            case 0: g_mouse_sens=std::max(0,std::min(3,g_mouse_sens+d)); break;
            case 1: g_sound_vol=std::max(0,std::min(100,g_sound_vol+d*10)); break;
            case 2: g_music_vol=std::max(0,std::min(100,g_music_vol+d*10)); break;
            }
        }
        break;
    default: break;
    }
    return MACT_NONE;
}

inline void menu_open_pause(GameState rs) {
    g_menu.current=MENU_PAUSE; g_menu.selectedItem=0;
    g_menu.returnState=rs; g_menu.deleteConfirm=false;
}

inline void menu_open_main() {
    g_menu.current=MENU_MAIN; g_menu.selectedItem=0; g_menu.deleteConfirm=false;
}
