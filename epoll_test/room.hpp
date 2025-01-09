#pragma once

#include <string>
#include <vector>
#include "question.hpp"
class Player; // forward declaration

class Room
{
public:
    bool gameStarted = false;
    static unsigned int next_room_id;
    int correctAnswer;
    unsigned int room_id;
    std::string name;
    std::vector<Player*> players_in_room;
    Player* leader = nullptr; // Dodaj ten wskaźnik
    Question currentQuestion;
    void addPlayerToRoom(Player* player_to_add);
    void listPlayers(int fd) const;
    void removePlayerFromRoom(Player* player_to_remove);
    // Dodaj metody do ustawiania i pobierania lidera
    void setLeader(Player* newLeader);
    Player* getLeader() const;

    void ProceedQuestion(std::string &squestion_str, int &correct);
};