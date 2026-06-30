#pragma once
#include <emscripten.h>
#include <emscripten/html5.h>
#include "Sprite.hpp"
#include "Locations.hpp"
#include "Utils.hpp"

class Weapon;

class Dummy : public Sprite{
public:
    Dummy(float* gravity,std::vector<Sprite*>& s)
        : Sprite(gravity,s)
    {
        rect={500,300,50,50};

        txt=SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            50,
            50
        );

        SDL_Texture* prev=SDL_GetRenderTarget(renderer);

        SDL_SetRenderTarget(renderer,txt);
        SDL_SetRenderDrawColor(renderer,0,0,255,255);
        SDL_RenderClear(renderer);

        SDL_SetRenderTarget(renderer,prev);

        hp=100;
    }

    void take_dmg(int dmg) override{
        alive_take_dmg(dmg);
        emscripten_log(1,"took damage");
    }

    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        setHurtbox();
        dy+=*gravity*dt;
        parryRect=rect;

        MoveAndHandleX(dt);
        MoveAndHandleY(dt);

        render(r);
    }

    ~Dummy(){
        SDL_DestroyTexture(txt);
    }
};

class Enemy:public Sprite{
    public:
    Sprite* target;
    float cd=0.f;
    bool dashin=false;
    float secondsPreparing=0.f;
    bool attackState=false;
    float maxSpeed=50.f;
    Enemy(float* gravity,std::vector<Sprite*>& s,Sprite* t):
    Sprite(gravity,s),target(t){
        rect={500,300,50,50};
        hp=100;
        Ai=true;
    }
    void take_dmg(int dmg) override{
        alive_take_dmg(dmg);
    }
    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        parryRect={rect.x-25,rect.y-25,rect.w+50,rect.h+50};
        setHurtbox();
        cd-=dt;
        if (cd<0) cd=0;
        dy+=*gravity*dt;
        if (target->rect.x>rect.x){
            if (target->rect.x-rect.x<200 && target->rect.y-rect.y<50 && target->rect.y-rect.y>-50 && cd==0)
                attackState=true;
            else 
                attackState=false;
            if (dx<50){
                dx+=50;
                dx=std::min(dx,maxSpeed);
            }
        }
        else if (target->rect.x<rect.x){
            if (target->rect.x-rect.x>-200 && target->rect.y-rect.y<50 && target->rect.y-rect.y>-50 && cd==0)
                attackState=true;
            else 
                attackState=false;
            if (dx>-50){
                dx-=50;
                dx=std::max(dx,-maxSpeed);
            }
        }
        if (attackState){
            emscripten_log(1,"Preparing attack");
            secondsPreparing+=dt;
            if (secondsPreparing>0.5f){
                emscripten_log(1,"Attack!");
                if (target->rect.x>rect.x)
                    dx+=400;
                else
                    dx-=400;
                secondsPreparing=0.f;
                attackState=false;
                dashin=true;
            }
        }
        else{
            secondsPreparing=0.f;
        }
        if (SDL_HasIntersectionF(&hurtRect,&target->rect) && cd==0 && dashin){
            sprites.push_back(new HitSprite(0.f,hurtRect,gravity,sprites,20,true,this,200.f));
            emscripten_log(1,"Spawned hitbox");
            dx-=dx*0.5f;
            cd=1.f;
            dashin=false;
        }
        MoveAndHandleX(dt);
        MoveAndHandleY(dt);
        render(r);
    }
    void render(SDL_Renderer* r) override{
        int color=(secondsPreparing/0.5f)*255;
        SDL_SetRenderDrawColor(r,color,color,255,255);
        SDL_RenderFillRectF(r,&rect);
    }
};

class Bullet:public Sprite{
    public:
    int dmg;
    std::vector<Sprite*> mustDamage;
    Sprite* dealer;
    bool parried=false;
    float speed;
    float knockback=200.f;
    Vec2f dest;
    Bullet(
        SDL_FRect r,float* g,
        std::vector<Sprite*>& s,int d,Sprite* de,
        Vec2f destination,float spd,float k=200.f)
    :Sprite(g,s),dmg(d),dealer(de),dest(destination),speed(spd),knockback(k){
        rect=r;
        collidable=false;
    }
    void render(SDL_Renderer* r) override{
        SDL_SetRenderDrawColor(r,255,255,0,255);
        SDL_RenderFillRectF(r,&rect);
    }
    void update(SDL_Renderer* r,float cd) override{
        ACTIVE_CHECK();
        setHurtbox();
        size_t count=sprites.size();
        emscripten_log(1,"Sprites size %d",count);
        for (size_t j=0;j<count;j++){
            if (sprites[j]==this || (sprites[j]==dealer && !parried)) 
                continue;
            if (SDL_HasIntersectionF(&sprites[j]->parryRect,&rect) && (sprites[j]->collidable || sprites[j]->hp>0)){
                mustDamage.push_back(sprites[j]);
                active=false;
                emscripten_log(1,"Bullet hit something");
            }
        }

        float distance =
            sqrtf((dest.x-rect.x)*(dest.x-rect.x)+
            (dest.y-rect.y)*(dest.y-rect.y));

        if (distance <= speed * cd){
            rect.x = dest.x;
            rect.y = dest.y;
            active = false;
            return;
        }
        Vec2f s=GetSpeed(rect,{dest.x,dest.y},speed);
        dx=s.x;
        dy=s.y;
        MoveAndHandleX(cd);
        MoveAndHandleY(cd);
        render(r);
    }
    void post_update(SDL_Renderer* r,float cd) override{
        if (mustDamage.size()<=0){
            return;
        }
        for (auto i:mustDamage){
            if (i->isParrying){
                dest=i->pointingTo;
                active=true;
                parried=true;
            }
            else{
                i->take_dmg(dmg);
                if (rect.x > i->rect.x){
                    i->dx-=knockback;
                }
                else{
                    i->dx+=knockback;
                }
            }
        }
        mustDamage.clear();
    }
};

class EnemyShooting:public Sprite{
    Sprite* target;
    float cd=0.f;
    public:
    EnemyShooting(float* gravity,std::vector<Sprite*>& s,Sprite* t):
    Sprite(gravity,s),target(t){
        rect={500,300,50,50};
        hp=100;
        Ai=true;
    }
    void render(SDL_Renderer* r) override{
        SDL_SetRenderDrawColor(r,255,0,255,255);
        SDL_RenderFillRectF(r,&rect);
    }
    void take_dmg(int dmg) override{
        alive_take_dmg(dmg);
    }
    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        setHurtbox();
        parryRect={rect.x-25,rect.y-25,rect.w+50,rect.h+50};
        cd-=dt;
        if (cd<0) cd=0;
        pointingTo={target->rect.x,target->rect.y};
        dy+=*gravity*dt;
        if (target->rect.x>rect.x){
            dx+=50;
            dx=std::min(dx,50.f);
        }
        else if (target->rect.x<rect.x){
            dx-=50;
            dx=std::max(dx,-50.f);
        }
        float distance=sqrtf((target->rect.x-rect.x)*(target->rect.x-rect.x)+(target->rect.y-rect.y)*(target->rect.y-rect.y));
        if (distance<550 && cd==0){
            sprites.push_back(
                new Bullet({hurtRect.x,hurtRect.y,10,10},
                gravity,sprites,20,this,pointingTo,400.f));
            emscripten_log(1,"Spawned bullet");
            cd=2.f;
        }
        MoveAndHandleX(dt);
        MoveAndHandleY(dt);
        render(r);
    }
};

class Brick:public Sprite{
    public:
    Brick(float* gravity,std::vector<Sprite*>& s):Sprite(gravity,s){
        rect={200,200,50,50};
        txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,50,50);
        SDL_Texture* prev=SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer,txt);  
        SDL_SetRenderDrawColor(renderer,0,255,0,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer,prev);
    }
    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        setHurtbox();
        parryRect=rect;
        render(r);
    }
};

class Player:public Sprite{
    public:
    Weapon* weapon;
    SDL_Texture* undmgdtxt;
    SDL_Texture* dmgdtxt;
    bool took_dmg=false;
    bool ShiftPressed=false;
    bool E_held=false;
    bool Mouse_held=false;
    bool sliding=false;
    float parryTimer;
    float parryCd=0.f;
    float DashTimer=0.f;
    float DashTime=0.25f;
    Player(float* gravity,std::vector<Sprite*>& s):Sprite(gravity,s){
        rect={100,100,50,50};
        undmgdtxt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,50,50);
        txt=undmgdtxt;
        dmgdtxt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,50,50);
        SDL_Texture* prev=SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer,txt);  
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer,0,255,0,255);
        SDL_FRect inner={0,20,10,10};
        SDL_RenderFillRectF(renderer,&inner);
        SDL_SetRenderTarget(renderer,dmgdtxt);
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer,prev);
    }
    /*void take_dmg(int dmg) override{
        if (DashTimer>0.f) return;
        alive_take_dmg(dmg);
        took_dmg=true;
    }*/
    void HandleInput(const Uint8* state,Uint32 mouseState);
    void update(SDL_Renderer* r,float dt) override;
    ~Player(){
        SDL_DestroyTexture(undmgdtxt);
        SDL_DestroyTexture(dmgdtxt);
    }
};

#include "Weapons.hpp"

inline void Player::HandleInput(const Uint8* state,Uint32 mouseState){
    pointingTo={mouseRect.x,mouseRect.y};
    int speed=200;
    if (state[SDL_SCANCODE_A]){
        if (dx>-speed)
            dx=-speed;
    }
    else if (state[SDL_SCANCODE_D]){
        if (dx<speed)
            dx=speed;
    }
    if (state[SDL_SCANCODE_W] && !midAir){
        dy=-250;
        midAir=true;
    }
    if (mouseState & SDL_BUTTON_LMASK){
        if (weapon) weapon->Use();
    }
    if (mouseState & SDL_BUTTON_RMASK){
        if (weapon) weapon->AltUse();
    }
    if (state[SDL_SCANCODE_R]){
        if (weapon) weapon->Reload();
    }
    if (state[SDL_SCANCODE_LSHIFT] && !ShiftPressed){
        DashTimer=DashTime;
        ShiftPressed=true;
    }
    else if (!state[SDL_SCANCODE_LSHIFT] && ShiftPressed){
        ShiftPressed=false;
    }
    isParrying=false;
    if (state[SDL_SCANCODE_E] && !E_held && parryCd == 0.f){
        parryTimer=1.f;
        parryCd=1.f;
        E_held=true;
    }
    else if (!state[SDL_SCANCODE_E]){
        E_held=false;
    }
}

inline void Player::update(SDL_Renderer* r,float dt){
    ACTIVE_CHECK();
    setHurtbox();
    parryCd-=dt;
    if (parryCd<0.f) parryCd=0.f;
    parryTimer-=dt;
    if (parryTimer<0.f) parryTimer=0.f;
    parryRect={rect.x-25,rect.y-25,rect.w+50,rect.h+50};
    const Uint8* state=SDL_GetKeyboardState(NULL);
    Uint32 mouseState=SDL_GetMouseState(NULL,NULL);
    HandleInput(state,mouseState);
    if (parryTimer>0){
        isParrying=true;
    }
    else{
        isParrying=false;
    }
    if (weapon){
        weapon->Update(dt);
        if (DashTimer<=0.f)
            weapon->Draw();
    }
    dy+=*gravity*dt;
    if (DashTimer>0.f){
        dy=0;
        dx=(dx>0?500:-500);
        DashTimer-=dt;
        collidable=false;
    }
    else{
        collidable=true;
    }
    MoveAndHandleX(dt);
    MoveAndHandleY(dt);

    if (DashTimer<=0.f)
        render(r);
    
    if (took_dmg){
        txt=dmgdtxt;
        took_dmg=false;
    }
    else{
        txt=undmgdtxt;
    }
}
