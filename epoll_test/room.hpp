#pragma once

#include <string>
#include <vector>
#include "question.hpp"
class Player; // forward declaration

class Room
{
public:
    static const size_t MAX_PLAYERS = 3; // Maximum number of players allowed

    bool gameStarted = false;
    static unsigned int next_room_id;
    int correctAnswer;
    unsigned int room_id;
    std::string name;
    std::vector<Player*> players_in_room;
    Player* leader = nullptr; // Dodaj ten wskaźnik
    Question currentQuestion;
    void addPlayerToRoom(Player* player_to_add);
    void removePlayerFromRoom(Player* player_to_remove);
    // Dodaj metody do ustawiania i pobierania lidera
    void setLeader(Player* newLeader);
    Player* getLeader() const;

    void playerAnswered();

    void ProceedQuestion(std::string &question_str, int &correct);

    bool bonusGiven = false; // Dodany atrybut do śledzenia bonusu

    // Dodanie deklaracji nowej funkcji do wyświetlania graczy wraz z ID pokoju
    void displayAllPlayers(int fd, bool flag) const;

private:
    unsigned int playersAnsweredCount = 0; // Initialize counter

};
//POMIEDZY RUNDAMI WYSYLAC RANKING /ranking. END na koncu. 
// sprawdzanie dlugosci gracza i pokoju nazwy
// maksymalna dlugiosc wiadomosci na czacie
//zapytać Igora o to jak czyta dlugość odebranych danych - spytać o to czy wszedzie gdzie trzeba jest znak END
//sprawdzic edge triggered
//492 nie obsługujecie tutaj sklejania/dzielenia komunikatów przez TCP
//sprawdzsic timeoput w epoll_wait
//sprawdzic czy jak wejdzie klient przed wyslaniem drugiego pytanie, to czy je dostanie
//dostaje od klienta odpowiedz + int czzasu ktory // 1000