#include <iostream>
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <vector>
#include <functional>
#include <map>
#include <any>
#include <cmath>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool running = true;

class Sprite{
    public:
    SDL_FRect rect;
    SDL_Texture* txt;
    float* gravity;
    bool collidable=true;
    bool active=true;
    float dx,dy;
    int hp=100;
    float weight=1.f;
    std::vector<Sprite*>& sprites;
    virtual void update(SDL_Renderer*,float dt){}
    void render(SDL_Renderer* r){
        SDL_RenderCopyF(r,txt,nullptr,&rect);
    }
    virtual ~Sprite()=default;
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
            SDL_FRect hitRect={w->owner->rect.x,w->owner->rect.y,
                w->owner->rect.w*2,w->owner->rect.h};
            for (int j=w->owner->sprites.size()-1;j>=0;j--){
                Sprite* i=w->owner->sprites[j];
                if (i!=w->owner){
                    if (SDL_HasIntersectionF(&w->owner->rect,&hitRect)){
                        i->hp-=dmg;
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
        render(r);
    }
};

class Player:public Sprite{
    public:
    Weapon* weapon;
    Player(float* gravity,std::vector<Sprite*>& s):Sprite(gravity,s){
        rect={100,100,50,50};
        txt=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGBA8888,SDL_TEXTUREACCESS_TARGET,50,50);
        SDL_Texture* prev=SDL_GetRenderTarget(renderer);
        SDL_SetRenderTarget(renderer,txt);  
        SDL_SetRenderDrawColor(renderer,255,0,0,255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer,prev);
    }
    void update(SDL_Renderer* r,float dt) override{
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
            dy=100;
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
        if (weapon){
            weapon->Update(dt);
            weapon->Draw();
        }
        dy+=*gravity*dt;
        MoveAndHandleX(dt);
        MoveAndHandleY(dt);
        render(r);
    }
    ~Player(){
        SDL_DestroyTexture(txt);
    }
};

std::vector<Sprite*> sprites;
float gravity=100.f;
float dt=0.f;
int start,end;
SDL_FRect mouseRect={0,0,10,10};

void HandleDeltaTime(){
    start=SDL_GetTicks();
    dt=(start-end)/1000.f;
    end=start;
    dt=SDL_min(dt,0.033f);
}

void update() {
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

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawColor(renderer,255,255,255,255);
    SDL_RenderFillRect(renderer,&mouseRect);

    for (auto sprite : sprites) {
        sprite->update(renderer, dt);
    }

    for (int i=sprites.size()-1;i>=0;i--){
        if (!sprites[i]->active){
            delete sprites[i];
            sprites.erase(sprites.begin()+i);
        }
    }

    SDL_RenderPresent(renderer);
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
        800,
        600,
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

    Player* p=new Player(&gravity,sprites);
    p->weapon=new Sword(p,[](Weapon*){});
    sprites.push_back(p);
    sprites.push_back(new Brick(&gravity,sprites));

    emscripten_set_main_loop(update, 0, 1);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
