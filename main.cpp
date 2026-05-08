#include <iostream>
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <vector>
#include <functional>

SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;
bool running = true;

class Sprite{
    public:
    SDL_FRect rect;
    SDL_Texture* txt;
    float* gravity;
    bool collidable=true;
    float dx,dy;
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
        rect.x+=dx*dt;
        for (auto i:sprites){
            if (i==this) continue;
            if (SDL_HasIntersectionF(&rect,&i->rect)){
                if (collidable&&i->collidable){
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
                    dx=0;
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
    Sprite* owner;
    std::function<void(Weapon*)> onUse;
    std::function<void(Weapon*)> reload;
    public:
    Weapon(Sprite* owner,std::function<void(Weapon*)> onUse,std::function<void(Weapon*)> reload):
        owner(owner),onUse(onUse),reload(reload){}
    void Use(){
        onUse(this);
    }
    void Reload(){
        reload(this);
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
        render(r);
    }
};

class Player:public Sprite{
    public:
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
            dx=-200;
        }
        else if (state[SDL_SCANCODE_D]){
            dx=200;
        }
        else{
            dx=0;
        }
        if (state[SDL_SCANCODE_SPACE]&&dy==0){
            dy=-500;
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
            default:
                break;
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    for (auto sprite : sprites) {
        sprite->update(renderer, dt);
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

    sprites.push_back(new Player(&gravity,sprites));
    sprites.push_back(new Brick(&gravity,sprites));

    emscripten_set_main_loop(update, 0, 1);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
