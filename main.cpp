#include <iostream>
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <vector>
#include <functional>
#include <map>
#include <any>
#include <cmath>
#define ACTIVE_CHECK() if (!active) return;

SDL_FRect mouseRect={0,0,10,10};

template <typename T>
struct Vec2{
    T x,y;
};
using Vec2f=Vec2<float>;

Vec2f GetSpeed(SDL_FRect& object, const SDL_FPoint& destination, float speed)
{
    float centerX = object.x + object.w * 0.5f;
    float centerY = object.y + object.h * 0.5f;

    float dx = destination.x - centerX;
    float dy = destination.y - centerY;

    float distance = sqrtf(dx * dx + dy * dy);

    if (distance <= 0.001f)
        return {0.f, 0.f};

    dx /= distance;
    dy /= distance;

    dx *= speed;
    dy *= speed;

    return {dx, dy};
}

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool running = true;

class Sprite;
class Particle;

Particle* CreateParticle(float* fravity,
    std::vector<Sprite*>& sprites,
    float x,
    float y);

void HitStop(SDL_Renderer*);

class Sprite{
    public:
    SDL_FRect rect;
    SDL_FRect hurtRect;
    SDL_Texture* txt;
    float* gravity;
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
        SDL_RenderCopyF(r,txt,nullptr,&rect);
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

class Room{
    public:
    std::vector<Sprite*> sprites;
    std::function<void(Room*,SDL_Renderer*,float)> Update;
    Room(std::function<void(Room*,SDL_Renderer*,float)> u):Update(u){}
    ~Room(){
        for (auto i:sprites){
            delete i;
        }
    }
    void update(SDL_Renderer* r,float dt){
        Update(this,r,dt);
    }
};

class Location{
    public:
    size_t curr;
    std::vector<Room*> rooms;
    Location(std::vector<Room*> r):rooms(r){}
    ~Location(){
        for (auto i:rooms){
            delete i;
        }
    }
    void go_forward(){
        if (curr<rooms.size()-1) curr++;
    }
    void go_back(){
        if (curr>0) curr--;
    }
    void update(SDL_Renderer* r,float dt){
        rooms[curr]->Update(rooms[curr],r,dt);
    }
};

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
                HitStop();
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
    
    Sword(Sprite* o,std::function<void(Weapon*)> draw):Weapon(o,
        [](Weapon* w){
            Sword* wep=dynamic_cast<Sword*>(w);
            if (wep->cd>0) return;
            SDL_FRect hitRect;
            if (wep->owner->dx>0)
                hitRect={w->owner->rect.x,w->owner->rect.y,
                    w->owner->rect.w*2,w->owner->rect.h};
            else if(wep->owner->dx<0)
                hitRect={w->owner->rect.x-w->owner->rect.w*2,w->owner->rect.y,
                    w->owner->rect.w*2,w->owner->rect.h};
            for (int j=w->owner->sprites.size()-1;j>=0;j--){
                Sprite* i=w->owner->sprites[j];
                if (i!=w->owner){
                    if (SDL_HasIntersectionF(&i->hurtRect,&hitRect)){
                        w->owner->sprites.push_back(new HitSprite(0.f,hitRect,w->owner->gravity,w->owner->sprites,wep->dmg,true,w->owner));
                    }
                }
            }
            if (w->owner->dx>0)
                w->owner->dx+=500;
            else if (w->owner->dx<0)
                w->owner->dx-=500;
            wep->cd=wep->maxCd;
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
            wep->cd=std::max(wep->cd,0.f);
        }
    ){
    }
    ~Sword()=default;
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
        SDL_SetRenderTarget(renderer,dmgdtxt);
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer,prev);
    }
    void take_dmg(int dmg) override{
        alive_take_dmg(dmg);
        took_dmg=true;
    }
    void update(SDL_Renderer* r,float dt) override{
        ACTIVE_CHECK();
        setHurtbox();
        parryCd-=dt;
        if (parryCd<0.f) parryCd=0.f;
        const Uint8* state=SDL_GetKeyboardState(NULL);
        if (state[SDL_SCANCODE_A]){
            if (dx>-200)
                dx=-200;
        }
        else if (state[SDL_SCANCODE_D]){
            if (dx<200)
                dx=200;
        }
        if (state[SDL_SCANCODE_SPACE] && dy==0){
            dy=-100;
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
            SDL_FRect parryRect={rect.x-20,rect.y-20,rect.w+40,rect.h+40};
            size_t count=sprites.size();
            for (int j=0;j<count;j++){
                Sprite* i=sprites[j];
                if (i!=this){
                    if (SDL_HasIntersectionF(&i->hurtRect,&parryRect)){
                        Vec2f speed=GetSpeed(i->rect,
                            {rect.x+rect.w/2.f,rect.y+rect.h/2.f},300.f);
                        isParrying=true;
                    }
                }
            }
            parryCd=0.5f;
            E_held=true;
        }
        else if (!state[SDL_SCANCODE_E]){
            E_held=false;
        }
        if (cd>0.f){
            SDL_FRect r={rect.x+rect.w+10,rect.y+Y,10,10};
            
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

float gravity=100.f;
float dt=0.f;
int start,end;

void HandleDeltaTime(){
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
    dt=SDL_min(dt,0.033f);
}

void update(Room* room, SDL_Renderer* r, float dt) {
    HandleDeltaTime();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
                break;
            case SDL_MOUSEMOTION:
                mouseRect.x+=event.motion.xrel;
                mouseRect.y+=event.motion.yrel;
                break;
            default:
                break;
        }
    }

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    size_t count=room->sprites.size();

    for (int i=0;i<count;i++) {
        room->sprites[i]->update(r, dt);
    }

    for (int i=0;i<count;i++) {
        room->sprites[i]->post_update(r, dt);
    }

    for (int i=count-1;i>=0;i--){
        if (!room->sprites[i]->active){
            delete room->sprites[i];
            room->sprites.erase(room->sprites.begin()+i);
        }
    }

    SDL_SetRenderDrawColor(r,255,255,255,255);
    SDL_RenderFillRectF(r,&mouseRect);

    SDL_RenderPresent(renderer);
}

Location* location;

void Update(){
    location->update(renderer,dt);
}

int main() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    window = SDL_CreateWindow(
        "SDL Emscripten",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1000,
        800,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );

    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    location=new Location({
        new Room(update)
    });

    Player* p=new Player(
        &gravity,location->rooms[0]->sprites);
    p->weapon=new Sword(p,[](Weapon*){});
    location->rooms[0]->sprites.push_back(p);
    location->rooms[0]->sprites.push_back(new Brick(
        &gravity,location->rooms[0]->sprites));
    Brick* b=new Brick(&gravity,location->rooms[0]->sprites);
    b->rect={0,700,1000,100};
    location->rooms[0]->sprites.push_back(b);
    location->rooms[0]->sprites.push_back(
        new Enemy(&gravity,location->rooms[0]->sprites,p));
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    emscripten_set_main_loop(Update, 0, 1);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
