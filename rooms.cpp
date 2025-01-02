#include "server_common.hpp"
#include <unistd.h> 
#include <string>
#include <iostream>
#include <vector>
#include <poll.h>
#include <cstring>
#include <unistd.h> // for read and write

using namespace std;

class Player; // forward declaration

class Room
{
public:
    unsigned int room_id;
    string name;
    static unsigned int next_room_id;
    // Store Player pointers (forward-declared above)
    vector<Player*> players_in_room;

    Room();
    void addPlayerToRoom(Player* player_to_add);
    void listPlayers();
};

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

class Player
{
    public:
        string name;
        unsigned int room_id = 0;
        int fd;
        pollfd pfd;
        bool nameAsked = false;

        Player(int newFd)
        {
            fd = newFd;
        }

        void askName()
        {
            string askMsg = "Podaj nick: ";
            write(fd, askMsg.c_str(), askMsg.length());
            char buf[64];
            ssize_t bytes_read = read(fd, buf, sizeof(buf) - 1);
            if (bytes_read > 0)
            {
                buf[bytes_read] = '\0'; // Null-terminate the string
                name = buf;
                nameAsked = true;
            }
        }

        void sendMenu()
        {
            string message = "Wybierz opcje: \n 1.Dołącz do pokoju. \n 2.Stwórz nowy pokój. \n";
            write(fd, message.c_str(), message.length());
        }

        void menuHandler(char input[16], vector<Room>& local_rooms)
        {

            string make_room_msg = "Podaj nazwę pokoju (max 16 znaków): ";
            string list_rooms_msg = "Lista dostępnych pokoi: ";
            string unknown_command = "Nieznana operacja";
            // int bytes_read = read(fd, buf, 31);
            // printf(buf);
            if(strncmp(input, "1", 1) == 0)
            {
                string room_choice_msg = "Dołącz do pokoju nr: ";
                char choice[8]{};
                cout << "Gracz " + name + "chce dołączyć do pokoju." << endl; 
                write(fd, room_choice_msg.c_str(), room_choice_msg.length());
                int bytes_read = read(fd, choice, 7);
                choice[bytes_read] = '\0';
                if(strncmp(choice, "1", 1) == 0)
                {
                    local_rooms[0].addPlayerToRoom(this);

                }
                
            }
            else if(strncmp(input, "2", 1) == 0)
            {
                write(fd, make_room_msg.c_str(), make_room_msg.length());
            }
            else
            {
                write(fd,unknown_command.c_str(), unknown_command.length());
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

void Room::listPlayers()
{
    cout << "Players in room " << name << " (ID: " << room_id << "):" << endl;
    for (const auto &player : players_in_room)
    {
        cout << "Player name: " << player->name << ", Player ID: " << player->fd << endl;
    }
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

    int fd1 = accept(server_fd, 0, 0);
    int fd2 = accept(server_fd, 0, 0);
    int fd3 = accept(server_fd, 0, 0);

    players.emplace_back(fd1);
    players.emplace_back(fd2);
    players.emplace_back(fd3);

    cout << "PETLA" << endl;
    for(auto &player : players)
    {
        char choice[16]{};
        player.askName();

        player.sendMenu();
        int bytes_read = read(player.fd, choice, 15);
        choice[bytes_read] = '\0';
        player.menuHandler(choice, rooms);
    }

    for(auto room : rooms)
    {
        room.listPlayers();
    }

}