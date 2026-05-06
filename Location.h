#ifndef LOCATION_H
#define LOCATION_H

#include <vector>
#include "Room.h"

class Location {
public:
    Location();
    ~Location();
    
    void addRoom(Room* room);
    void removeRoom(Room* room);
    std::vector<Room*>& getRooms();
    Room* getRoom(size_t index);
    size_t getRoomCount() const;

private:
    std::vector<Room*> rooms;
};

#endif // LOCATION_H
