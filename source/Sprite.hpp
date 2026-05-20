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
    SDL_FRect hurtRect;
    SDL_Texture* txt;
    float* gravity;
    bool midAir=false;
    bool collidable=true;
    bool active=true;
    float dx,dy;
    int hp=100;
    int mustGetDmg=0;
    float weight=1.f;
    bool isParrying=false;
    std::vector<Sprite*>& sprites;
    virtual void update(SDL_Renderer*,float dt){ACTIVE_CHECK();}
    virtual void take_dmg(int dmg){mustGetDmg+=dmg;}
    void receiveDmg(){hp-=mustGetDmg;}
    virtual void post_update(SDL_Renderer*,float dt){receiveDmg();mustGetDmg=0;}
    virtual void render(SDL_Renderer* r){
        if (dx>0){
            SDL_RenderCopyExF(r,txt,NULL,&rect,0,NULL,SDL_FLIP_HORIZONTAL);
        }
        else if (dx<0){
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
            dx-=1000*dt;
            if (dx<0) dx=0;
        }
        else if (dx<0){
            dx+=1000*dt;
            if (dx>0) dx=0;
        }
        rect.x+=dx*dt;
        for (auto i:sprites){
            if (i==this) continue;
            if (SDL_HasIntersectionF(&rect,&i->rect)){
                if (collidable && i->collidable){
                    if (dx>0){
                        if (weight>i->weight){
                            i->rect.x=rect.x+rect.w;
                        }
                        else{
                            rect.x=i->rect.x-rect.w;
                        }
                    }
                    else{
                        if (weight>i->weight){
                            i->rect.x=rect.x-rect.w;
                        }
                        else{
                            rect.x=i->rect.x+rect.w;
                        }
                    }
                }
            }
        }
    }
    void MoveAndHandleY(float dt){
        rect.y+=dy*dt;
        for (auto i:sprites){
            if (i==this) continue;
            if (SDL_HasIntersectionF(&rect,&i->rect)){
                if (collidable&&i->collidable){
                    midAir=false;
                    if (dy>0){
                        if (weight>i->weight){
                            i->rect.y=rect.y+rect.h;
                        }
                        else{
                            rect.y=i->rect.y-rect.h;
                        }
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
