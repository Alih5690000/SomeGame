#include "Location.h"
#include <algorithm>

Location::Location() : currentRoom(nullptr) {
}

Location::~Location() {
}

void Location::addRoom(Room* room) {
    if (room != nullptr) {
        rooms.push_back(room);
    }
}

void Location::removeRoom(Room* room) {
    auto it = std::find(rooms.begin(), rooms.end(), room);
    if (it != rooms.end()) {
        rooms.erase(it);
    }
}

std::vector<Room*>& Location::getRooms() {
    return rooms;
}

Room* Location::getRoom(size_t index) {
    if (index < rooms.size()) {
        return rooms[index];
    }
    return nullptr;
}

size_t Location::getRoomCount() const {
    return rooms.size();
}

void Location::setCurrentRoom(Room* room) {
    currentRoom = room;
}

Room* Location::getCurrentRoom() {
    return currentRoom;
}

void Location::update(SDL_Renderer* renderer, float deltaTime) {
    if (currentRoom != nullptr) {
        currentRoom->update(renderer, deltaTime);
    }
}
