#pragma once
#include "Utils.hpp"
#include <SDL2/SDL.h>
#include <vector>
#include "Globals.hpp"

class Sprite;
class Particle;

Particle* CreateParticle(float* fravity,
    std::vector<Sprite*>& sprites,
    float x,
    float y);

class Sprite{
    public:
    SDL_FRect rect;
    bool Ai=false;
    Vec2f pointingTo;
    float jumpboost=-150.f;
    SDL_FRect hurtRect;
    SDL_Texture* txt;
    float* gravity;
    float friction=1000.f;
    bool midAir=false;
    bool collidable=true;
    bool active=true;
    float dx,dy;
    int hp=100;
    float lastDx=0.f;
    int mustGetDmg=0;
    float weight=1.f;
    bool isParrying=false;
    std::vector<Sprite*>& sprites;
    virtual void update(SDL_Renderer*,float dt){ACTIVE_CHECK();}
    virtual void take_dmg(int dmg){mustGetDmg+=dmg;}
    void receiveDmg(){hp-=mustGetDmg;}
    virtual void post_update(SDL_Renderer*,float dt){receiveDmg();mustGetDmg=0;}
    virtual void render(SDL_Renderer* r){
        if (lastDx>0){
            SDL_RenderCopyExF(r,txt,NULL,&rect,0,NULL,SDL_FLIP_HORIZONTAL);
        }
        else if (lastDx<0){
            SDL_RenderCopyExF(r,txt,NULL,&rect,0,NULL,SDL_FLIP_NONE);
        }
        else{
            SDL_RenderCopyExF(r,txt,NULL,&rect,0,NULL,SDL_FLIP_NONE);
        }
    }
    virtual ~Sprite()=default;
    void setHurtbox(){
        hurtRect={rect.x-10,rect.y-10,rect.w+20,rect.h+20};
    }
    void alive_take_dmg(int dmg){
        hp-=dmg;

        for (int i=0;i<10;i++){
            sprites.push_back(
                (Sprite*)CreateParticle(
                    gravity,
                    sprites,
                    rect.x+rect.w/2,
                    rect.y+rect.h/2
                )
            );
        }
        if (hp<=0) active=false;
    }
    Sprite(float* gravity,std::vector<Sprite*>& s):
        gravity(gravity),sprites(s){}
    void MoveAndHandleX(float dt){
        if (dx>0){
            lastDx=dx;
            if (midAir){
                dx-=friction*dt/4;
            }
            else{
                dx-=friction*dt;
            }
            if (dx<0) dx=0;
        }
        else if (dx<0){
            lastDx=dx;
            if (midAir){
                dx+=friction*dt/4;
            }
            else{
                dx+=friction*dt;
            }
            if (dx>0) dx=0;
        }
        rect.x+=dx*dt;
        size_t count=sprites.size();
        for (size_t j=0;j<count;j++){
            Sprite* i=sprites[j];
            if (i==this) continue;
            if (SDL_HasIntersectionF(&rect,&i->rect)){
                if (collidable && i->collidable){
                    if (dx > 0){
                        if (weight > i->weight){
                            i->rect.x = rect.x + rect.w;
                        }
                        else{
                            rect.x = i->rect.x - rect.w;
                        }
                    }
                    else if (dx < 0){
                        if (weight > i->weight){
                            i->rect.x = rect.x - i->rect.w;
                        }
                        else{
                            rect.x = i->rect.x + i->rect.w;
                        }
                    }
                    if (Ai && !midAir){
                        if (dy >= 0){
                            dy=jumpboost;
                            midAir=true;
                        }
                    }
                }
            }
        }
    }
    void MoveAndHandleY(float dt){
        rect.y+=dy*dt;
        midAir=true;
        for (size_t j=0;j<sprites.size();j++){
            Sprite* i=sprites[j];
            if (i==this) continue;
            if (SDL_HasIntersectionF(&rect,&i->rect)){
                if (collidable&&i->collidable){
                    if (dy>0){
                        if (weight>i->weight){
                            i->rect.y=rect.y+rect.h;
                        }
                        else{
                            rect.y=i->rect.y-rect.h;
                        }
                        midAir=false;
                    }
                    else{
                        if (weight>i->weight){
                            i->rect.y=rect.y-rect.h;
                        }
                        else{
                            rect.y=i->rect.y+rect.h;
                        }
                    }
                    dy=0;
                }
            }
        }
    }
};

class HitSprite:public Sprite{
    public:
    float lifeTime;
    bool wasParried=false;
    int dmg;
    Sprite* dealer;
    bool oneFramed=false;
    size_t cycles=0;
    std::vector<Sprite*> mustDamage;
    HitSprite(float l,SDL_FRect r,float* g,std::vector<Sprite*>& s,int d,bool o,Sprite* de)
    :Sprite(g,s),lifeTime(l),dmg(d),oneFramed(o),dealer(de){
        rect=r;
        collidable=false;
    }
    void update(SDL_Renderer* r,float cd) override{
        ACTIVE_CHECK();
        setHurtbox();
        if (oneFramed){
            if (cycles>0){
                active=false;
                return;
            }
        }
        else{
            lifeTime-=cd;
            if (lifeTime<0.f) {
                active=false;
                return;
            }
        }
        size_t count=sprites.size();
        for (size_t j=0;j<count;j++){
            if (sprites[j]==this || sprites[j]==dealer) continue;
            if (SDL_HasIntersectionF(&sprites[j]->hurtRect,&rect)){
                mustDamage.push_back(sprites[j]);
                emscripten_log(1,"Hit something");
            }
        }
        cycles++;
        emscripten_log(1,"Hitbox cycle");
    }
    void post_update(SDL_Renderer* r,float cd) override{
        for (auto i:mustDamage){
            if (i->isParrying){
                dealer->take_dmg(dmg);
                wasParried=true;
                hitStopTime=0.5f;
                active=false;
            }
            else{
                i->take_dmg(dmg);
            }
        }
        mustDamage.clear();
    }
};

class Particle : public Sprite{
public:
    float life=0.5f;

    Particle(
        float* gravity,
        std::vector<Sprite*>& s,
        float x,
        float y
    ) : Sprite(gravity,s)
    {
        rect={x,y,6,6};

        txt=SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            6,
            6
        );

        SDL_Texture* prev=SDL_GetRenderTarget(renderer);

        SDL_SetRenderTarget(renderer,txt);
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderClear(renderer);

        SDL_SetRenderTarget(renderer,prev);

        collidable=false;

        dx=(rand()%400)-200;
        dy=-(rand()%300);
    }

    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        life-=dt;

        if (life<=0){
            active=false;
            return;
        }

        dy+=*gravity*dt;

        rect.x+=dx*dt;
        rect.y+=dy*dt;

        render(r);
    }

    ~Particle(){
        SDL_DestroyTexture(txt);
    }
};

Particle* CreateParticle(float* gravity,
    std::vector<Sprite*>& sprites,
    float x,
    float y){
        return new Particle(gravity,sprites,x,y);
}
