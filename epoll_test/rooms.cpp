#include "server_common.hpp"
#include "room.hpp"
#include "question.hpp"
#include <unistd.h> 
#include <string>
#include <iostream>
#include <vector>
#include <poll.h>
#include <cstring>
#include <unistd.h> // for read and write
#include <fcntl.h> // Add this include for fcntl
#include <sys/epoll.h> // Ensure epoll is included
#include <algorithm>

using namespace std;

vector<Room> rooms;
unsigned int Room::next_room_id = 1;

void printRooms(int fd)
{
    unsigned short int iterator = 1;
    for (const auto &room : rooms)
    {
        cout << "Room ID: " << room.room_id << ", Name: " << room.name << endl;
        if(fd > 2)
        {
            string msg = to_string(iterator) + " name: " + room.name + "\n";
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
    InRoom
};

class Player
{
public:
    string name;
    unsigned int room_id = 0;
    int fd;
    pollfd pfd;
    PlayerState state = PlayerState::AwaitingName; // Initialize to AwaitingName
    string readBuffer; // Buffer to accumulate incoming data
    Room* room_in = nullptr;
    

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

    void handleInput(const string& input, vector<Room>& local_rooms)
    {
        switch(state)
        {
            case PlayerState::AwaitingName:
                name = input;
                printf("Nowy Gracz: %s\n", name.c_str());
                sendMenu();
                state = PlayerState::AwaitingMenu;
                break;

            case PlayerState::AwaitingMenu:
                menuHandler(input, local_rooms);
                break;

            case PlayerState::ChoosingRoom:
                {
                    unsigned int room_number = atoi(input.c_str());
                    if (room_number < 1 || room_number >= Room::next_room_id)
                    {
                        string error_msg = "Nieprawidłowy numer pokoju. Wybierz numer od 1 do " + to_string(Room::next_room_id - 1) + ".\n";
                        write(fd, error_msg.c_str(), error_msg.length());
                    }
                    else
                    {
                        local_rooms[room_number-1].addPlayerToRoom(this);
                        // sendMenu(); // Usuń tę linię, aby nie wysyłać menu po dołączeniu do pokoju
                        state = PlayerState::InRoom;
                    }
                }
                break;

            case PlayerState::CreatingRoom:
                {
                    //this->name = input;
                    local_rooms.emplace_back();
                    local_rooms.back().name = input;
                    local_rooms.back().room_id = Room::next_room_id++;
                    
                    // Dodanie gracza do nowo utworzonego pokoju
                    local_rooms.back().addPlayerToRoom(this);
                    
                    // Powiadomienie gracza o utworzeniu i dołączeniu do pokoju
                    write(fd, "Pokój został utworzony i dołączono do niego.\n", 50);
                    
                    // Zmiana stanu na InRoom
                    state = PlayerState::InRoom;
                }
                break;
            
            case PlayerState::InRoom:
                {
                    if (input == "/leave") {
                        leaveRoom(local_rooms);
                    }
                    else if (input == "/start")
                    {
                        if (room_in && room_in->getLeader() == this)
                        {
                            sendMessageToRoom("ZACZYNAMY za 3 2 1!", local_rooms, true);
                            Question q;
                            int correct = q();
                            cout << to_string(correct) << endl;

                        }
                        else
                        {
                            string errorMsg = "Tylko lider może rozpocząć grę.\n";
                            write(fd, errorMsg.c_str(), errorMsg.length());
                        }
                    }
                    else if (input == "/listrooms") printRooms(fd);
                    else if (input == "/listplayers") 
                    {
                        if (room_in != nullptr)
                        {
                            room_in->listPlayers(fd);
                        }
                        else
                        {
                            string errorMsg = "Nie jesteś w żadnym pokoju.\n";
                            write(fd, errorMsg.c_str(), errorMsg.length());
                        }
                    }
                    else {
                        sendMessageToRoom(input, local_rooms);
                    }
                }
                break;
        }
    }

    void leaveRoom(vector<Room>& local_rooms)
    {
        // Znajdź pokój, do którego gracz należy
        Room* currentRoom = nullptr;
        for (auto &room : local_rooms)
        {
            if (room.room_id == room_id)
            {
                currentRoom = &room;
                break;
            }
        }

        if (currentRoom)
        {
            // Usuń gracza z pokoju
            currentRoom->removePlayerFromRoom(this);

            // Powiadomienie innych graczy w pokoju
            string leaveMsg = name + " opuścił pokój.\n";
            for (const auto& player : currentRoom->players_in_room)
            {
                write(player->fd, leaveMsg.c_str(), leaveMsg.length());
            }

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

    void sendMessageToRoom(const string& message, vector<Room>& local_rooms, bool server_notification = false)
    {
        // Find the room the player is currently in
        Room* currentRoom = nullptr;
        for (auto &room : local_rooms)
        {
            if (room.room_id == room_id)
            {
                currentRoom = &room;
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

    void menuHandler(const string& input, vector<Room>& local_rooms)
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
    for (const auto& player : players_in_room)
    {
        write(player->fd, leaderMsg.c_str(), leaderMsg.length());
    }
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

    // Optionally, notify the joining player of existing members
    string welcomeMsg = "Witaj w pokoju " + name + "!\n";
    write(player_to_add->fd, welcomeMsg.c_str(), welcomeMsg.length());

    // Ustaw lidera jeśli to pierwszy gracz
    if (players_in_room.size() == 1)
    {
        setLeader(player_to_add);
    }
}

void Room::listPlayers(int fd) const
{
    // Zmienna do przechowywania pojedynczej linii wiadomości
    string line = "Players in room " + name + " (ID: " + to_string(room_id) + "):\n";
    write(fd, line.c_str(), line.length());

    for (const auto &player : players_in_room)
    {
        if (player == leader)
        {
            line = "Player name: " + player->name + " (lider), Player ID: " + to_string(player->fd) + "\n";
        }
        else
        {
            line = "Player name: " + player->name + ", Player ID: " + to_string(player->fd) + "\n";
        }
        write(fd, line.c_str(), line.length());
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
        // Znajdź indeks pokoju w globalnej liście
        extern vector<Room> rooms; // Upewnij się, że 'rooms' jest zadeklarowane jako extern
        auto room_it = std::find_if(rooms.begin(), rooms.end(),
            [this](const Room& r) { return r.room_id == this->room_id; });

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

int main(int argc, char **argv)
{
    
    vector<Player> players;

    //printRooms(0);

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
    //char welcomeMsg[] = "Witaj graczu! Podaj imie: \n";

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

                    Player newPlayer(player_fd);
                    newPlayer.sendNamePrompt(); // Prompt for name
                    players.push_back(newPlayer);

                    // Add new socket to epoll
                    epoll_event client_event;
                    client_event.data.fd = player_fd;
                    client_event.events = EPOLLIN | EPOLLET;
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
                    [client_fd](const Player& p) { return p.fd == client_fd; });

                if (it != players.end())
                {
                    char buffer[512];
                    ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
                    if (bytes_read == -1)
                    {
                        if (errno != EAGAIN && errno != EWOULDBLOCK)
                        {
                            perror("read");
                            it->quitGame();
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                            players.erase(it);
                        }
                        // Otherwise, no data to read
                        continue;
                    }
                    else if (bytes_read == 0)
                    {
                        // Client disconnected
                        it->quitGame();
                        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL);
                        players.erase(it);
                        continue;
                    }
                    else
                    {
                        // Accumulate data
                        it->readBuffer.append(buffer, bytes_read);

                        // Check for newline or end of input
                        size_t pos;
                        while ((pos = it->readBuffer.find('\n')) != string::npos)
                        {
                            string input = it->readBuffer.substr(0, pos);
                            it->readBuffer.erase(0, pos + 1);

                            // Trim carriage return if present
                            if (!input.empty() && input.back() == '\r')
                                input.pop_back();

                            it->handleInput(input, rooms);
                        }
                    }
                }
            }
        }
    }

    close(epoll_fd);
    close(server_fd);
    return 0;
}