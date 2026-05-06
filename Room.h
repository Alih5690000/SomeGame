#ifndef ROOM_H
#define ROOM_H

#include <functional>
#include <vector>
#include <SDL2/SDL.h>
#include "Sprite.h"

class Room {
public:
    Room();
    ~Room();
    
    void update(SDL_Renderer* renderer, float deltaTime);
    void addSprite(Sprite* sprite);
    void removeSprite(Sprite* sprite);
    std::vector<Sprite*>& getSprites();
    
    void setUpdateFunction(std::function<void(SDL_Renderer*, float)> func);

private:
    std::function<void(SDL_Renderer*, float)> update;
    std::vector<Sprite*> sprites;
};

#endif // ROOM_H
