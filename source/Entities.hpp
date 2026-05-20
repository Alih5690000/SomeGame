#include <emscripten.h>
#include <emscripten/html5.h>
#include "Sprite.hpp"
#include "Locations.hpp"

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

        MoveAndHandleX(dt);
        MoveAndHandleY(dt);

        render(r);
    }

    ~Dummy(){
        SDL_DestroyTexture(txt);
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
        for (int j=0;j<count;j++){
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

class Enemy:public Sprite{
    public:
    Sprite* target;
    float cd=0.f;
    bool dashin=false;
    float secondsPreparing=0.f;
    bool attackState=false;
    Enemy(float* gravity,std::vector<Sprite*>& s,Sprite* t):
    Sprite(gravity,s),target(t){
        rect={500,300,50,50};
    }
    void take_dmg(int dmg) override{
        alive_take_dmg(dmg);
    }
    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        setHurtbox();
        cd-=dt;
        if (cd<0) cd=0;
        dy+=*gravity*dt;
        if (target->rect.x>rect.x){
            if (target->rect.x-rect.x<200 && target->rect.y-rect.y<50 && target->rect.y-rect.y>-50 && cd==0)
                attackState=true;
            else 
                attackState=false;
            if (dx<50)
                dx+=50;
        }
        else if (target->rect.x<rect.x){
            if (target->rect.x-rect.x>-200 && target->rect.y-rect.y<50 && target->rect.y-rect.y>-50 && cd==0)
                attackState=true;
            else 
                attackState=false;
            if (dx>-50)
                dx-=50;
        }
        if (attackState){
            emscripten_log(1,"Preparing attack");
            secondsPreparing+=dt;
            if (secondsPreparing>0.5f){
                emscripten_log(1,"Attack!");
                if (target->rect.x>rect.x)
                    dx=600;
                else
                    dx=-600;
                secondsPreparing=0.f;
                attackState=false;
                dashin=true;
            }
        }
        else{
            secondsPreparing=0.f;
        }
        if (SDL_HasIntersectionF(&hurtRect,&target->rect) && cd==0 && dashin){
            sprites.push_back(new HitSprite(0.f,hurtRect,gravity,sprites,20,true,this));
            emscripten_log(1,"Spawned hitbox");
            dx=-dx*0.5f;
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
    ~Enemy(){
        SDL_DestroyTexture(txt);
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
        render(r);
    }
};

class Player:public Sprite{
    public:
    Weapon* weapon;
    SDL_Texture* undmgdtxt;
    SDL_Texture* dmgdtxt;
    bool took_dmg=false;
    float Y=0;
    float E_held=false;
    float parryTimer;
    float parryCd=0.f;
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
    void take_dmg(int dmg) override{
        alive_take_dmg(dmg);
        took_dmg=true;
    }
    void HandleParry(){
        SDL_FRect parryRect={rect.x-50,rect.y-50,rect.w+100,rect.h+100};
        size_t count=sprites.size();
        for (int j=0;j<count;j++){
            Sprite* i=sprites[j];
            if (i!=this){
                if (SDL_HasIntersectionF(&i->hurtRect,&parryRect)){
                    Vec2f speed=GetSpeed(i->rect,
                        {rect.x+rect.w/2.f,rect.y+rect.h/2.f},300.f);
                    parryTimer=0.5f;
                }
            }
        }
        parryCd=0.5f;
        E_held=true;
    }
    void HandleInput(const Uint8* state){
        if (state[SDL_SCANCODE_A]){
            if (dx>-200)
                dx=-200;
        }
        else if (state[SDL_SCANCODE_D]){
            if (dx<200)
                dx=200;
        }
        if (state[SDL_SCANCODE_W] && !midAir){
            dy=-250;
            midAir=true;
        }
        if (state[SDL_SCANCODE_F]){
            if (weapon) weapon->Use();
        }
        if (state[SDL_SCANCODE_G]){
            if (weapon) weapon->AltUse();
        }
        if (state[SDL_SCANCODE_R]){
            if (weapon) weapon->Reload();
        }
        isParrying=false;
        if (state[SDL_SCANCODE_E] && !E_held){
            HandleParry();
        }
        else if (!state[SDL_SCANCODE_E]){
            E_held=false;
        }
    }
    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        setHurtbox();
        parryCd-=dt;
        if (parryCd<0.f) parryCd=0.f;
        parryTimer-=dt;
        if (parryTimer<0.f) parryTimer=0.f;
        const Uint8* state=SDL_GetKeyboardState(NULL);
        HandleInput(state);
        if (parryTimer>0){
            isParrying=true;
        }
        else{
            isParrying=false;
        }
        if (parryCd>0.f){
            Y+=dt*rect.w*2;
            float X=0;
            if (dx<0){
                X=rect.x-10;
            }
            else if (dx>0){
                X=rect.x+rect.w+10;
            }
            else{
                X=rect.x+rect.w/2;
            }
            SDL_FRect re={X,rect.y+Y,20,20};
            SDL_SetRenderDrawColor(r,0,0,255,255);
            SDL_RenderFillRectF(r,&re);
        }
        else{
            Y=0;
        }
        if (weapon){
            weapon->Update(dt);
            weapon->Draw();
        }
        dy+=*gravity*dt;
        MoveAndHandleX(dt);
        MoveAndHandleY(dt);
        if (took_dmg){
            txt=dmgdtxt;
            took_dmg=false;
        }
        else{
            txt=undmgdtxt;
        }
        render(r);
    }
    ~Player(){
        SDL_DestroyTexture(undmgdtxt);
        SDL_DestroyTexture(dmgdtxt);
    }
};