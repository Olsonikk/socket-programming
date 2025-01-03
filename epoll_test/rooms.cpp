#include "server_common.hpp"
#include "room.hpp"
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
    for (const auto &room : rooms)
    {
        cout << "Room ID: " << room.room_id << ", Name: " << room.name << endl;
        if(fd > 2)
        {
            string msg = to_string(room.room_id) + " name: " + room.name + "\n";
            write(fd, msg.c_str(), msg.length());
        }
    }
   
}

enum class PlayerState {
    AwaitingName,
    AwaitingMenu,
    ChoosingRoom,
    CreatingRoom
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
    bool nameAsked = false;

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
        string message = "Wybierz opcje: \n 1.Dołącz do pokoju. \n 2.Stwórz nowy pokój. \n";
        write(fd, message.c_str(), message.length());
    }

    void handleInput(const string& input, vector<Room>& local_rooms)
    {
        switch(state)
        {
            case PlayerState::AwaitingName:
                name = input;
                nameAsked = true;
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
                        sendMenu();
                        state = PlayerState::AwaitingMenu;
                    }
                }
                break;

            case PlayerState::CreatingRoom:
                {
                    this->name = input;
                    local_rooms.emplace_back();
                    local_rooms.back().name = input;
                    local_rooms.back().room_id = Room::next_room_id++;
                    write(fd, "Pokój został utworzony.\n", 24);
                    sendMenu();
                    state = PlayerState::AwaitingMenu;
                }
                break;
        }
    }

    void menuHandler(const string& input, vector<Room>& local_rooms)
    {
        string make_room_msg = "Podaj nazwę pokoju (max 16 znaków): ";
        string unknown_command = "Nieznana operacja\n";

        if(input == "1")
        {
            state = PlayerState::ChoosingRoom;
            printRooms(fd); // Display the list of rooms to the client
            write(fd, "Dołącz do pokoju nr: ", 22);
        }
        else if(input == "2")
        {
            state = PlayerState::CreatingRoom;
            write(fd, make_room_msg.c_str(), make_room_msg.length());
        }
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

Room::Room()
{
    cout << "Podaj nazwę pokoju: ";
    cin >> name;
    room_id = next_room_id++;
    cout << "Stworzono pokój o nazwie " + name << " z ID " << room_id << endl;
}

void Room::addPlayerToRoom(Player* player_to_add)
{
    players_in_room.push_back(player_to_add);
    cout << "dodadno gracza do pokoju" << endl;
    player_to_add->room_id = room_id;

    for (const auto &player : players_in_room)
    {
        cout << "Player name: " << player->name << ", Player ID: " << player->fd << endl;
    }
}

void Room::listPlayers() const
{
    cout << "Players in room " << name << " (ID: " << room_id << "):" << endl;
    for (const auto &player : players_in_room)
    {
        cout << "\t Player name: " << player->name << ", Player ID: " << player->fd << endl;
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

    rooms.emplace_back(); // tworzy obiekt Room i od razu dodaje go do wektora
    rooms.emplace_back();
    rooms.emplace_back();

    // cout << "ID pokoju 1: " << rooms[0].room_id << endl;
    // cout << "ID pokoju 2: " << rooms[1].room_id << endl;
    // cout << "ID pokoju 3: " << rooms[2].room_id << endl;

    printRooms(0);

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