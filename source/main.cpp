#include <iostream>
#include <SDL2/SDL.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <vector>
#include <functional>
#include <map>
#include <any>
#include <cmath>
#include "Utils.hpp"
#include "Sprite.hpp"
#include "Locations.hpp"
#include "Globals.hpp"
#include "Entities.hpp"
#include "Weapons.hpp"

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

    if (hitStopTime>0.f){
        hitStopTime-=dt;
        if (hitStopTime<0.f) hitStopTime=0.f;
        SDL_RenderPresent(r);
        return;
    }

    spawncd1-=dt;

    SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
    SDL_RenderClear(r);

    size_t count=room->sprites.size();

    for (size_t i=0;i<count;i++) {
        room->sprites[i]->update(r, dt);
    }

    for (size_t i=0;i<count;i++) {
        room->sprites[i]->post_update(r, dt);
    }

    for (size_t i=count; i-->0;){
        if (!room->sprites[i]->active){
            delete room->sprites[i];
            room->sprites.erase(room->sprites.begin()+i);
        }
    }

    if (spawncd1<=0.f){
        Enemy* e=new Enemy(&gravity,room->sprites,room->sprites[0]);
        e->rect={300.f+rand()%1000,100.f,50.f,50.f};
        room->sprites.push_back(e);
        spawncd1=5.f;
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
    srand(time(0));
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
    for (int i=0;i<5;i++){
        Enemy* e=new Enemy(&gravity,location->rooms[0]->sprites,p);
        e->rect={300.f+rand()%1000,100.f,50.f,50.f};
        location->rooms[0]->sprites.push_back(e);
    }
    {
        Brick* b=new Brick(&gravity,location->rooms[0]->sprites);
        b->rect={400,675,100,25};
        location->rooms[0]->sprites.push_back(b);
        b=new Brick(&gravity,location->rooms[0]->sprites);
        b->rect={700,675,300,25};
        location->rooms[0]->sprites.push_back(b);
    }
    SDL_ShowCursor(SDL_DISABLE);
    SDL_SetRelativeMouseMode(SDL_TRUE);

    emscripten_set_main_loop(Update, 0, 1);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
