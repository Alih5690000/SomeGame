#pragma once
#include <functional>
#include "Sprite.hpp"
#include "Entities.hpp"
#include "Globals.hpp"
#include <math.h>

class Weapon{
    public:
    Sprite* owner;
    SDL_Texture* body;
    SDL_Texture* arm;
    SDL_Texture* head;
    Vec2f Arm_MovingOffset;
    Vec2f Head_MovingOffset;
    Vec2f ArmCenter;
    Vec2f HeadCenter;
    std::function<void(Weapon*)> onUse;
    std::function<void(Weapon*)> onAltUse;
    std::function<void(Weapon*)> reload;
    std::function<void(Weapon*)> draw;
    std::function<void(Weapon*,float)> update;
    Weapon(Sprite* owner,std::function<void(Weapon*)> onUse,
        std::function<void(Weapon*)> onAltUse,
        std::function<void(Weapon*)> reload,
        std::function<void(Weapon*)> draw,
        std::function<void(Weapon*,float)> update):
            owner(owner),onUse(onUse),onAltUse(onAltUse),reload(reload),draw(draw),update(update){
                //temporary decision
                body=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,owner->rect.w,owner->rect.h);
                arm=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET
                    ,10,10);
                head=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET
                    ,5,5);
                SDL_Texture* prev=SDL_GetRenderTarget(renderer);
                SDL_SetRenderTarget(renderer,body);
                SDL_SetRenderDrawColor(renderer,255,0,0,0);
                SDL_RenderClear(renderer);
                SDL_SetRenderTarget(renderer,arm);
                SDL_SetRenderDrawColor(renderer,0,255,0,0);
                SDL_RenderClear(renderer);
                SDL_SetRenderTarget(renderer,head);
                SDL_SetRenderDrawColor(renderer,0,0,255,0);
                SDL_RenderClear(renderer);
                SDL_SetRenderTarget(renderer,prev);
            }
    virtual ~Weapon()=default;
    //In development
    void _Draw(){
        SDL_RenderCopyF(renderer,body,NULL,&owner->rect);
        float angle=std::atan2(owner->pointingTo.y-(owner->rect.y+owner->rect.h/2.f),
                owner->pointingTo.x-(owner->rect.x+owner->rect.w/2.f))*180.f/3.14159f;
        SDL_FPoint p={ArmCenter.x,ArmCenter.y};
        if (owner->dx==0.f){
            SDL_RenderCopyExF(renderer,arm,NULL,&owner->rect,angle,&p,SDL_FLIP_NONE);
        }
        if (owner->dx<0.f){
            SDL_FRect rect={owner->rect.x-Arm_MovingOffset.x,owner->rect.y-Arm_MovingOffset.y,owner->rect.w,owner->rect.h};
            SDL_RenderCopyExF(renderer,arm,NULL,&rect,angle,&p,SDL_FLIP_NONE);
        }
        if (owner->dx>0.f){
            SDL_FRect rect={owner->rect.x+Arm_MovingOffset.x,owner->rect.y+Arm_MovingOffset.y,owner->rect.w,owner->rect.h};
            SDL_RenderCopyExF(renderer,arm,NULL,&rect,angle,&p,SDL_FLIP_NONE);
        }
    }
    void Use(){
        onUse(this);
    }
    [[deprecated("Dont call this one pls, implement render through angle and body and arm texture")]]
    void Draw(){
        _Draw();
    }
    void AltUse(){
        onAltUse(this);
    }
    void Update(float dt){
        update(this,dt);
    }
    void Reload(){
        reload(this);
    }
};

class Sword:public Weapon{
    public:
    float cd=0.f;
    float maxCd=0.5f;
    float swordLength=40.f;
    float swordWidth=10.f;
    int dmg=100;
    float sinceLastHit=0.f;
    int combo=0;
    bool PlayerMode=false;
    SDL_FRect hitRect;
    
    Sword(Sprite* o):Weapon(o,
        [this](Weapon* w){
            Sword* wep=dynamic_cast<Sword*>(w);
            if (wep->cd>0.f) return;
            if (wep->owner->pointingTo.x>
                wep->owner->rect.x+wep->owner->rect.w/2.f)
                wep->hitRect={w->owner->rect.x,w->owner->rect.y,
                    w->owner->rect.w*2,w->owner->rect.h};
            else if(wep->owner->pointingTo.x<
                wep->owner->rect.x+wep->owner->rect.w/2.f)
                wep->hitRect={w->owner->rect.x-w->owner->rect.w*2,w->owner->rect.y,
                    w->owner->rect.w*2,w->owner->rect.h};
            if (wep->sinceLastHit<1.5f && wep->cd==0.f){
                wep->combo++;
            }
            else{
                wep->combo=0;
            }
            float k=50.f;
            if (wep->combo==0){
                emscripten_log(1,"First hit");
                wep->dmg=100;
                wep->maxCd=0.5f;
            }
            else if (wep->combo==1){
                emscripten_log(1,"Second hit");
                wep->dmg=110;
                k=150.f;
                wep->maxCd=0.75f;
            }
            else{
                emscripten_log(1,"Third hit");
                wep->dmg=250.f;
                k=300.f;
                if (wep->owner->pointingTo.x>
                    wep->owner->rect.x)
                    wep->owner->dx+=500.f;
                else
                    wep->owner->dx-=500.f;
                wep->combo=0;
                wep->maxCd=1.5f;
            }
            wep->cd=wep->maxCd;
            wep->sinceLastHit=0.f;
            w->owner->sprites.push_back(new HitSprite(0.f,
                wep->hitRect,w->owner->gravity,
                w->owner->sprites,wep->dmg,true,w->owner,k));
        },
        [](Weapon* w){}
        ,[](Weapon* w){}
        ,[](Weapon* w){
            Sword* wep=dynamic_cast<Sword*>(w);
            float angle=0.f;
            if (wep->cd>0){
                angle=360.f*(1.f-wep->cd/wep->maxCd);
            }
            float radians=angle*3.14159f/180.f;
            float sx=w->owner->rect.x+w->owner->rect.w/2.f;
            float sy=w->owner->rect.y+w->owner->rect.h/2.f;
            float ex=sx+std::cos(radians)*wep->swordLength;
            float ey=sy+std::sin(radians)*wep->swordLength;
            SDL_SetRenderDrawColor(renderer,255,255,0,255);
            SDL_RenderDrawLineF(renderer,sx,sy,ex,ey);
        },
        [](Weapon* w,float dt){
            Sword* wep=dynamic_cast<Sword*>(w);
            wep->cd-=dt;
            wep->sinceLastHit+=dt;
            wep->cd=std::max<float>(wep->cd,0.f);
        }
    ){
    }
    ~Sword()=default;
};

class Gun:public Weapon{
    public:
    float cd=0.f;
    float maxCd=0.75f;
    int dmg=50;
    SDL_Texture* txt;
    Gun(Sprite* o):Weapon(o,
        [this](Weapon* w){
            Gun* wep=dynamic_cast<Gun*>(w);
            if (wep->cd>0.f) return;
            wep->owner->sprites.push_back(new Bullet(
                {wep->owner->rect.x+wep->owner->rect.w/2.f,wep->owner->rect.y+wep->owner->rect.h/2.f,10,10},wep->owner->gravity,
                wep->owner->sprites,wep->dmg,wep->owner,wep->owner->pointingTo,1000));
            wep->cd=wep->maxCd;
        },
        [](Weapon* w){}
        ,[](Weapon* w){}
        ,[](Weapon* w){
            Gun* wep = dynamic_cast<Gun*>(w);

            float cx = wep->owner->rect.x + wep->owner->rect.w / 2.f;
            float cy = wep->owner->rect.y + wep->owner->rect.h / 2.f;

            float dx = wep->owner->pointingTo.x - cx;
            float dy = wep->owner->pointingTo.y - cy;

            float angleRad = std::atan2(dy, dx);
            float angleDeg = angleRad * 180.f / 3.14159f;

            if (wep->owner->pointingTo.x > wep->owner->rect.x + wep->owner->rect.w / 2.f) {
                SDL_RenderCopyExF(
                renderer,
                wep->txt,
                NULL,
                &wep->owner->rect,
                angleDeg,
                NULL,
                SDL_FLIP_NONE
                );
            }
            else{
                SDL_RenderCopyExF(
                renderer,
                wep->txt,
                NULL,
                &wep->owner->rect,
                angleDeg,
                NULL,
                SDL_FLIP_VERTICAL
                );
            }
        }
        ,[](Weapon* w,float dt){
            Gun* wep=dynamic_cast<Gun*>(w);
            wep->cd-=dt;
            wep->cd=std::max<float>(wep->cd,0.f);
        }
    ){
        SDL_FRect FirstRect={0,0,5,10};
        SDL_FRect SecondRect={0,0,20,5};
        txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,20,20);
        SDL_SetTextureBlendMode(txt,SDL_BLENDMODE_BLEND);
        SDL_Texture* prev=SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer,txt);
        SDL_SetRenderDrawColor(renderer,0,0,0,0);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,255,255,0,255);
        SDL_RenderFillRectF(renderer,&FirstRect);
        SDL_RenderFillRectF(renderer,&SecondRect);
        SDL_SetRenderTarget(renderer,prev);
    }
    ~Gun()=default;
};
