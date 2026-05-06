#include "Room.h"
#include <algorithm>

Room::Room() : update(nullptr) {
}

Room::~Room() {
}

void Room::update(SDL_Renderer* renderer, float deltaTime) {
    if (update) {
        update(renderer, deltaTime);
    }
}

void Room::addSprite(Sprite* sprite) {
    if (sprite != nullptr) {
        sprites.push_back(sprite);
    }
}

void Room::removeSprite(Sprite* sprite) {
    auto it = std::find(sprites.begin(), sprites.end(), sprite);
    if (it != sprites.end()) {
        sprites.erase(it);
    }
}

std::vector<Sprite*>& Room::getSprites() {
    return sprites;
}

void Room::setUpdateFunction(std::function<void(SDL_Renderer*, float)> func) {
    update = func;
}
