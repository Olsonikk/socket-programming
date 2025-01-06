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
    Player* leader = nullptr; // Dodaj ten wskaźnik

    void addPlayerToRoom(Player* player_to_add);
    void listPlayers(int fd) const;
    void removePlayerFromRoom(Player* player_to_remove);

    // Dodaj metody do ustawiania i pobierania lidera
    void setLeader(Player* newLeader);
    Player* getLeader() const;
};