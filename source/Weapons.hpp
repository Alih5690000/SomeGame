#pragma once
#include <functional>
#include "Sprite.hpp"
#include "Entities.hpp"
#include "Globals.hpp"

class Weapon{
    public:
    Sprite* owner;
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
            owner(owner),onUse(onUse),onAltUse(onAltUse),reload(reload),draw(draw),update(update){}
    virtual ~Weapon()=default;
    void Use(){
        onUse(this);
    }
    void Draw(){
        draw(this);
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
    
    Sword(Sprite* o,std::function<void(Weapon*)> draw):Weapon(o,
        [](Weapon* w){
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
            w->owner->sprites.push_back(new HitSprite(0.f,
                wep->hitRect,w->owner->gravity,
                w->owner->sprites,wep->dmg,true,w->owner));
            if (wep->sinceLastHit<1.5f && wep->cd==0.f){
                wep->combo++;
            }
            else{
                wep->combo=0;
            }
            if (wep->combo==0){
                wep->maxCd=1.f;
                wep->dmg=100;
            }
            else if (wep->combo==1){
                wep->maxCd=0.5f;
                wep->dmg=110;
            }
            else if (wep->combo==2){
                wep->maxCd=0.35f;
                wep->dmg=130;
            }
            else{
                wep->maxCd=0.25f;
                wep->dmg=160;
            }
            wep->cd=wep->maxCd;
            wep->sinceLastHit=0.f;
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
            float ex=sx+cosf(radians)*wep->swordLength;
            float ey=sy+sinf(radians)*wep->swordLength;
            SDL_SetRenderDrawColor(renderer,255,255,0,255);
            SDL_RenderDrawLineF(renderer,sx,sy,ex,ey);
        },
        [](Weapon* w,float dt){
            Sword* wep=dynamic_cast<Sword*>(w);
            wep->cd-=dt;
            wep->sinceLastHit+=dt;
            wep->cd=std::max(wep->cd,0.f);
        }
    ){
    }
    ~Sword()=default;
};
