#include "server_common.hpp"
#include "room.hpp"
#include "question.hpp"
#include <unistd.h> 
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <poll.h>
#include <cstring>
#include <fcntl.h> // Add this include for fcntl
#include <sys/epoll.h> // Ensure epoll is included
#include <algorithm>
#include <list>
#include <memory>

using namespace std;

list<Player*> players; 

vector<shared_ptr<Room>> rooms;
unsigned int Room::next_room_id = 1;



void printRooms(int fd)
{
    unsigned short int iterator = 1;
    for (auto &room : rooms)
    {
        cout << "Room ID: " << room->room_id << ", Name: " << room->name << endl;
        if(fd > 2)
        {
            string msg = to_string(iterator) + " name: " + room->name + " game status:" + to_string(room->gameStarted) + " players: " + to_string(room->players_in_room.size()) + "/" + to_string(Room::MAX_PLAYERS) + "\n";
            write(fd, msg.c_str(), msg.length());
            iterator++;
        }
    }
   
}


enum class PlayerState {
    AwaitingName,
    AwaitingMenu,
    ChoosingRoom,
    CreatingRoom,
    InRoom,
    AwaitingAnswer
};

class Player
{
public:
    string name;
    unsigned int room_id = 0;
    int fd;
    PlayerState state = PlayerState::AwaitingName; // Initialize to AwaitingName
    string readBuffer; // Buffer to accumulate incoming data
    Room* room_in = nullptr;

    int points = 0; // Dodany atrybut do przechowywania punktów
    int big_points = 0;
    Player(int newFd)
    {
        fd = newFd;
    }

    void sendNamePrompt()
    {
        string askMsg = "Podaj nick: ";
        write(fd, askMsg.c_str(), askMsg.length());
    }

    void sendMenu()
    {
        string message = "Wybierz opcje: \n 1.Dołącz do pokoju. \n 2.Stwórz nowy pokój. \n 3. Lista Pokoi. \n";
        write(fd, message.c_str(), message.length());
    }

    bool isNameTaken(const string& name) {
    for (const auto& player : players) {
        if (player->name == name) {
            return true;
        }
    }
    return false;
    }

    // Zmień sygnaturę handleInput tak by przyjmowała vector<shared_ptr<Room>>&
    void handleInput(const string& input, vector<shared_ptr<Room>>& local_rooms)
    {
        switch(state)
        {
            case PlayerState::AwaitingName:
                if (isNameTaken(input)) {
                    string error_msg = "Nazwa użytkownika jest już zajęta. Podaj inną nazwę: ";
                    write(fd, error_msg.c_str(), error_msg.length());
                }
                else {
                    name = input;
                    printf("Nowy Gracz: %s\n", name.c_str());
                    sendMenu();
                    state = PlayerState::AwaitingMenu;
                }
                break;

            case PlayerState::AwaitingMenu:
                menuHandler(input);
                break;

            case PlayerState::ChoosingRoom:
                {
                    if(input == "/back")
                    {
                        sendMenu();
                        state = PlayerState::AwaitingMenu;
                        break;
                    } 
                    unsigned int room_number = atoi(input.c_str());
                    if (room_number < 1 || room_number >= Room::next_room_id)
                    {
                        string error_msg = "Nieprawidłowy numer pokoju. Wybierz numer od 1 do " + to_string(Room::next_room_id - 1) + ".\n";
                        write(fd, error_msg.c_str(), error_msg.length());
                        write(fd, "Dołącz do pokoju nr: ", 22);
                        // Remain in ChoosingRoom state
                    }
                    else
                    {
                        auto selectedRoom = local_rooms[room_number - 1];
                        if(selectedRoom->players_in_room.size() >= Room::MAX_PLAYERS)
                        {
                            string error_msg = "Pokój jest pełny. Nie możesz dołączyć.\n";
                            write(fd, error_msg.c_str(), error_msg.length());
                            write(fd, "Dołącz do pokoju nr: ", 22);
                            // Remain in ChoosingRoom state
                        }
                        else
                        {
                            selectedRoom->addPlayerToRoom(this);
                            state = PlayerState::InRoom;
                            if (selectedRoom->gameStarted)
                            {
                              state = PlayerState::AwaitingAnswer;
                            }
                        }
                    }
                }
                break;

            case PlayerState::CreatingRoom:
                {
                    if(input == "/back")
                    {
                        sendMenu();
                        state = PlayerState::AwaitingMenu;
                        break;
                    }

                    if (input.size() > 16 || input.size() <=0) {
                        string tooLongMsg = "Nazwa pokoju nie może mieć więcej niż 16 znaków.\nPodaj nazwę pokoju (max 16 znaków): ";
                        write(fd, tooLongMsg.c_str(), tooLongMsg.size());
                        // Zostajemy w stanie CreatingRoom
                        break;
                    }

                    // Sprawdzenie, czy nazwa pokoju jest już używana:
                    bool nameTaken = false;
                    for (auto &r : local_rooms) {
                        if (r->name == input) {
                            nameTaken = true;
                            break;
                        }
                    }
                    if (nameTaken) {
                        string error_msg = "Nazwa pokoju jest już zajęta.\n";
                        write(fd, error_msg.c_str(), error_msg.size());
                        sendMenu();
                        state = PlayerState::AwaitingMenu;
                        break;
                    }

                    local_rooms.emplace_back(std::make_shared<Room>());
                    local_rooms.back()->name = input;
                    local_rooms.back()->room_id = Room::next_room_id++;
                    local_rooms.back()->addPlayerToRoom(this);
                    state = PlayerState::InRoom;
                    break;
                }
                break;
            
            case PlayerState::InRoom:
                {
                    if (input == "/leave") {
                        big_points = 0;
                        leaveRoom(local_rooms);
                    }
                    else if (input == "/start")
                    {
                        if (room_in && room_in->getLeader() == this)
                        {
                            if (room_in->players_in_room.size() < 2) {
                                string errorMsg = "Nie można rozpocząć gry. Potrzebnych jest co najmniej dwóch graczy.\n";
                                write(fd, errorMsg.c_str(), errorMsg.length());
                                printf("Gracz '%s' próbował rozpocząć grę, ale jest tylko %zu graczy w pokoju.\n", name.c_str(), room_in->players_in_room.size());
                            }
                            else {
                                room_in->gameStarted = true;
                                printf("Gracz '%s' rozpoczął grę w pokoju '%s'.\n", name.c_str(), room_in->name.c_str());
                                string question_str;
                                room_in->ProceedQuestion(question_str, room_in->correctAnswer);

                                for (auto& player : room_in->players_in_room) {
                                    room_in->displayAllPlayers(player->fd, false);
                                }

                                sendMessageToRoom("Pytanie: " + question_str, local_rooms, true);
                                // Set state to AwaitingAnswer for all players in the room
                                
                                for (auto& player : room_in->players_in_room) {
                                    player->state = PlayerState::AwaitingAnswer;
                                    printf("Gracz '%s' ma teraz stan AwaitingAnswer.\n", player->name.c_str());
                                }
                            }
                        }
                        else
                        {
                            string errorMsg = "Tylko lider może rozpocząć grę.\n";
                            write(fd, errorMsg.c_str(), errorMsg.length());
                            printf("Gracz '%s' próbował rozpocząć grę, ale nie jest liderem.\n", name.c_str());
                        }
                    }
                    else if (input == "/listrooms") printRooms(fd);
                    else if (input == "/listplayers") 
                    {
                       room_in->displayAllPlayers(fd, true);
                    }
                    else if (input == "/ranking")
                    {
                        room_in->displayAllPlayers(fd, true);
                    }
                    else {
                        sendMessageToRoom(input, local_rooms);
                    }
                }
                break;

            case PlayerState::AwaitingAnswer:
                {
                    
                    if (room_in) {
                        cout << input.c_str() << endl;
                        Question q = room_in->currentQuestion;
                        int answer = 0;
                        int bonus_time = 0;
                        istringstream iss(input);
                        iss >> answer >> bonus_time;
                        if (answer == q.getAnswer())
                        {
                            write(fd, "Dobra odpowiedź!\n", 19);
                            points += 1; // Przyznanie 1 punktu za poprawną odpowiedź
                            points += bonus_time;

                            // Sprawdzenie, czy bonus został już przyznany
                            if (!room_in->bonusGiven) {
                                points += 2; // Przyznanie bonusowego punktu
                                room_in->bonusGiven = true;
                            }
                        }
                        else
                        {
                            write(fd,"Zła odpowiedź!\n", 18);
                            // Nie przyznawaj punktów za złą odpowiedź
                        }
                        state = PlayerState::InRoom;
                        room_in->playerAnswered(); // Increment counter
                    }
                    else {
                        string errorMsg = "Błąd: Nie jesteś w żadnym pokoju.\n";
                        write(fd, errorMsg.c_str(), errorMsg.length());
                    }
                }
                break;
        }
    }

    // Zmień sygnaturę leaveRoom tak by przyjmowała vector<shared_ptr<Room>>&
    void leaveRoom(vector<shared_ptr<Room>>& local_rooms)
    {
        // Znajdź pokój, do którego gracz należy
        shared_ptr<Room> currentRoom = nullptr;
        for (auto &room : local_rooms)
        {
            if (room->room_id == room_id)
            {
                currentRoom = room;
                break;
            }
        }

        if (currentRoom)
        {
            // Usuń gracza z pokoju
            currentRoom->removePlayerFromRoom(this);
            // Zmień stan gracza na AwaitingMenu
            state = PlayerState::AwaitingMenu;
            sendMenu();

        }
        else
        {
            // Pokój nie został znaleziony
            string errorMsg = "Błąd: Nie jesteś w żadnym pokoju.\n";
            write(fd, errorMsg.c_str(), errorMsg.length());
        }
    }

    // Zmień sygnaturę sendMessageToRoom tak by przyjmowała vector<shared_ptr<Room>>&
    void sendMessageToRoom(const string& message, vector<shared_ptr<Room>>& local_rooms, bool server_notification = false)
    {
        // Find the room the player is currently in
        shared_ptr<Room> currentRoom = nullptr;
        for (auto &room : local_rooms)
        {
            if (room->room_id == room_id)
            {
                currentRoom = room;
                break;
            }
        }

        if (currentRoom)
        {
            string formattedMsg;
            // Format the message with the player's name
            if(!server_notification)  //send a message to the room's chat
            {
                formattedMsg = this->name + ": " + message + "\n";
            }
            else
            {
                 formattedMsg = message + "\n";
            }
            

            // Broadcast the message to all players in the room
            for (const auto& player : currentRoom->players_in_room)
            {
                if(player->fd == fd && !server_notification) continue;
                write(player->fd, formattedMsg.c_str(), formattedMsg.length());
            }
        }
        else
        {
            // Handle case where room is not found
            string errorMsg = "Błąd: Nie znaleziono pokoju.\n";
            write(fd, errorMsg.c_str(), errorMsg.length());
        }
    }

    void menuHandler(const string& input)
    {
        string make_room_msg = "Podaj nazwę pokoju (max 16 znaków): ";
        string unknown_command = "Nieznana operacja\n";
        string no_rooms_msg = "Brak pokoi\n";
        if(input == "1")
        {
            if(Room::next_room_id <= 1) //dodac sprawdzanie czy są wolne
            {
                write(fd, no_rooms_msg.c_str(), no_rooms_msg.length());
                state = PlayerState::AwaitingMenu;
                return;
            }
            state = PlayerState::ChoosingRoom;
            printRooms(fd); // Display the list of rooms to the client
            write(fd, "Dołącz do pokoju nr: ", 22);
        }
        else if(input == "2")
        {

            state = PlayerState::CreatingRoom;
            write(fd, make_room_msg.c_str(), make_room_msg.length());
        }
        else if(input == "3") printRooms(fd);
        else
        {
            write(fd, unknown_command.c_str(), unknown_command.length());
        }
    }

    void quitGame()
    {
        shutdown(fd, SHUT_RDWR);
        close(fd);
        printf("Gracz %s opuscil gre.\n", name.c_str());
    }
};


void Room::setLeader(Player* newLeader)
{
    leader = newLeader;
    string leaderMsg = leader->name + " został(a) wybrany(a) na lidera pokoju.\n";
    cout << leaderMsg << endl;
}

Player* Room::getLeader() const
{
    return leader;
}

void Room::addPlayerToRoom(Player* player_to_add)
{
    players_in_room.push_back(player_to_add);
    cout << "dodano gracza do pokoju" << endl;
    player_to_add->room_id = room_id;
    player_to_add->room_in = this;
    // Broadcast join message to all players in the room except the one joining
    string joinMsg = player_to_add->name + " dołączył do pokoju.\n";
    for (const auto& player : players_in_room)
    {
        if (player->fd != player_to_add->fd)
        {
            write(player->fd, joinMsg.c_str(), joinMsg.length());
        }
    }

    // Ustaw lidera jeśli to pierwszy gracz
    if (players_in_room.size() == 1)
    {
        setLeader(player_to_add);
    }
}

void Room::ProceedQuestion(string &question_str, int &correctAnswer)
{
    currentQuestion = Question();
    correctAnswer = currentQuestion.getAnswer(); //correct answer
    question_str = currentQuestion.getQuestionText();
    cout << correctAnswer << endl;
    cout << question_str << endl;
    playersAnsweredCount = 0; // Reset counter
    bonusGiven = false; // Reset bonus flag for new question
}   

void Room::playerAnswered()
{
    playersAnsweredCount++;
    printf("Player answered. Count: %u/%zu\n", playersAnsweredCount, players_in_room.size());
    
    if (playersAnsweredCount >= players_in_room.size())
    {
        // Sprawdź, czy któryś z graczy osiągnął 10 punktów
        vector<Player*> winners;
        for (auto &p : players_in_room)
        {
            if (p->points >= 10)
            {
                winners.push_back(p);
            }
            this->displayAllPlayers(p->fd, false);
        }

        if (!winners.empty())
        {
            // Każdy zwycięzca dostaje +1 big_points
            for (auto &w : winners)
            {
                w->big_points++;
            }
            // Ogłoś zwycięzcę i zakończ grę
            string endMsg = "Koniec gry!\n";
            for (auto &p : players_in_room)
            {
                p->points = 0;
                write(p->fd, endMsg.c_str(), endMsg.size());
                this->displayAllPlayers(p->fd, true);
            }
            gameStarted = false;
            printf("Gra w pokoju '%s' została zakończona.\n", name.c_str());
            
        }
        else
        {
            // Resetuj liczenie odpowiedzi i generuj nowe pytanie
            playersAnsweredCount = 0;
            bonusGiven = false;
            string question_str;
            int nextAnswer;
            ProceedQuestion(question_str, nextAnswer);

            // Rozgłoś nowo wygenerowane pytanie
            string nextQ = "Pytanie: " + question_str + "\n";
            for (auto &p : players_in_room)
            {
                p->state = PlayerState::AwaitingAnswer;
                write(p->fd, nextQ.c_str(), nextQ.size());
            }
        }
    }
}

void Room::removePlayerFromRoom(Player* player_to_remove)
{
    // Znajdź i usuń gracza z listy uczestników
    auto it = std::find(players_in_room.begin(), players_in_room.end(), player_to_remove);
    if (it != players_in_room.end())
    {
        players_in_room.erase(it);
        cout << "Gracz " << player_to_remove->name << " został usunięty z pokoju " << name << "." << endl;
        string leaveMsg = player_to_remove->name + " opuścił pokój.\n";
        for (const auto& player : players_in_room)
        {
            write(player->fd, leaveMsg.c_str(), leaveMsg.length());
        }
    }

    // Jeśli usunięto lidera, ustaw nowego lidera
    if (player_to_remove == leader)
    {
        if (!players_in_room.empty())
        {
            setLeader(players_in_room.front());
        }
        else
        {
            leader = nullptr;
        }
    }

    // Jeśli pokój jest pusty, opcjonalnie usuń pokój z listy
    if (players_in_room.empty())
    {
        cout << "USUNIETO POKOJ" <<endl;
        // Znajdź indeks pokoju w globalnej liście
        extern vector<shared_ptr<Room>> rooms; // Upewnij się, że 'rooms' jest zadeklarowane jako extern
        auto room_it = std::find_if(rooms.begin(), rooms.end(),
            [this](const shared_ptr<Room>& r) { return r->room_id == this->room_id; });

        if (room_it != rooms.end())
        {
            cout << "Pokój " << name << " jest pusty i zostanie usunięty." << endl;
            rooms.erase(room_it);
            Room::next_room_id--;
        }
    }
}

// Function to set a file descriptor to non-blocking mode
int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void Room::displayAllPlayers(int fd, bool which_points) const
{
    string message = "Pokój ID: " + to_string(room_id) + ", Nazwa: " + name + "\n";
    message += "Lista graczy:\n";
    for (auto* player : players_in_room)
    {
        bool isLider = (player == leader);
        int showPoints = which_points ? player->big_points : player->points;
        message += " - " + player->name + (isLider ? " (lider), Points: " : ", Points: ")
            + to_string(showPoints) + "\n";
    }
    message += "END\n";
    write(fd, message.c_str(), message.size());
}

int main(int argc, char **argv)
{
    
    int server_fd = socket_bind_listen(argc, argv, SOCK_STREAM);

    // Set server socket to non-blocking
    setNonBlocking(server_fd);

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1 failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    // Add server socket to epoll
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        perror("epoll_ctl: server_fd");
        exit(EXIT_FAILURE);
    }

    const int MAX_EVENTS = 10;
    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == server_fd)
            {
                while (true)
                {
                    sockaddr_in client_addr;
                    socklen_t client_addr_size = sizeof(client_addr);
                    int player_fd = accept(server_fd, (sockaddr *)&client_addr, &client_addr_size);
                    if (player_fd == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // No more connections
                        else
                        {
                            perror("accept");
                            break;
                        }
                    }

                    setNonBlocking(player_fd); // Set client socket to non-blocking

                    printf("Nowy gracz dolaczyl do gry.\n");

                    // Tworzenie dynamicznej instancji gracza
                    Player* newPlayer = new Player(player_fd);
                    newPlayer->sendNamePrompt(); // Prompt for name
                    players.push_back(newPlayer);

                    // Add new socket to epoll
                    epoll_event client_event;
                    client_event.data.fd = player_fd;
                    client_event.events = EPOLLIN; 
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, player_fd, &client_event) == -1) {
                        perror("epoll_ctl: player_fd");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else
            {
                // Handle client events
                int client_fd = events[i].data.fd;
                auto it = std::find_if(players.begin(), players.end(),
                    [client_fd](Player* p) { return p->fd == client_fd; });

                if (it != players.end())
                {
                    char buffer[512];
                    while (true)
                    {
                        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
                        if (bytes_read == -1)
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                                break; // No more data to read
                            else
                            {
                                perror("read");
                                (*it)->quitGame();
                                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                                delete *it; // Zwolnienie pamięci
                                players.erase(it);
                                break;
                            }
                        }
                        else if (bytes_read == 0)
                        {
                            // Client disconnected
                            if ((*it)->room_in)
                            {
                                (*it)->room_in->removePlayerFromRoom(*it);
                            }
                            (*it)->quitGame();
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            delete *it;
                            players.erase(it);
                            break;
                        }
                        else
                        {
                            // Accumulate data
                            (*it)->readBuffer.append(buffer, bytes_read);

                            // Check for newline or end of input
                            size_t pos;
                            while ((pos = (*it)->readBuffer.find('\n')) != string::npos)
                            {
                                string input = (*it)->readBuffer.substr(0, pos);
                                (*it)->readBuffer.erase(0, pos + 1);

                                // Trim carriage return if present
                                if (!input.empty() && input.back() == '\r')
                                    input.pop_back();

                                (*it)->handleInput(input, rooms);
                            }
                        }
                    }
                }
            }
        }
    }

    for (auto player : players) {
        delete player;
    }

    close(epoll_fd);
    close(server_fd);
    return 0;
}
