#pragma once

#include <string>
#include <vector>

class Player; // forward declaration

class Room
{
public:
    static unsigned int next_room_id;

    unsigned int room_id;
    std::string name;
    std::vector<Player*> players_in_room;

    //Room();
    void addPlayerToRoom(Player* player_to_add);
    void listPlayers() const;
    void removePlayerFromRoom(Player* player_to_remove);
};