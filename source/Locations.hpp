#include <functional>
#include "Sprite.hpp"
#include "Utils.hpp"


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
