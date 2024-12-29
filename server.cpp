#include "server_common.hpp"
#include <poll.h>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h> // for read and write

using namespace std;

class Player
{
    public:
        string name;
        unsigned int room_id = 0;
        int fd;
        pollfd pfd;
        bool nameAsked = false;

        void askName()
        {
            char buf[64];
            ssize_t bytes_read = read(this->fd, buf, sizeof(buf) - 1);
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

        void menuHandler(char input[16])
        {

            string make_room_msg = "Podaj nazwę pokoju (max 16 znaków): ";

            // int bytes_read = read(fd, buf, 31);
            // printf(buf);
            if(strncmp(input, "2", 1) == 0)
            {
                write(fd, make_room_msg.c_str(), make_room_msg.length());
            }
        }

        void quitGame()
        {
            shutdown(fd, SHUT_RDWR);
            close(fd);
            printf("Gracz %s opuscil gre.\n", name.c_str());
        }
};



int main(int argc, char **argv)
{
    int servSock = socket_bind_listen(argc, argv, SOCK_STREAM);
    char welcomeMsg[] = "Witaj graczu! Podaj imie: \n";

    std::vector<Player> players;
    pollfd servPfd = {servSock, POLLIN, 0};
    Player serverPlayer;
    serverPlayer.fd = servSock;
    serverPlayer.pfd = servPfd;
    players.push_back(serverPlayer);

    while (true)
    {
        std::vector<pollfd> pfds;
        for (const auto& p : players)
        {
            pfds.push_back(p.pfd);
        }

        int ready = poll(pfds.data(), pfds.size(), -1);
        if (ready == -1)
        {
            perror("problem z poll.");
            exit(1);
        }

        if (pfds[0].revents & POLLIN)
        {
            sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int player_fd = accept(servSock, (sockaddr *)&client_addr, &client_addr_size);
            if (player_fd == -1)
            {
                perror("problem z accept.");
                exit(1);
            }
            printf("Nowy gracz dolaczyl do gry.\n");
            write(player_fd, welcomeMsg, sizeof(welcomeMsg));
            Player newPlayer;
            newPlayer.fd = player_fd;
            newPlayer.pfd = {player_fd, POLLIN | POLLOUT, 0};
            players.push_back(newPlayer);
        }

        for (size_t i = 1; i < players.size(); ++i)
        {
            if (pfds[i].revents & POLLIN)
            {
                if (!players[i].nameAsked)
                {
                    players[i].askName();
                    printf("Nowy Gracz: %s\n", players[i].name.c_str());
                    players[i].sendMenu();
                }
                else
                {
                    char buffer[16]{};
                    ssize_t bytes_read = read(players[i].fd, buffer, sizeof(buffer));
                    if (bytes_read == 0)
                    {
                        players[i].quitGame();
                        players.erase(players.begin() + i);
                        --i;
                    }
                    else{
                        players[i].menuHandler(buffer);
                    }
                    
                    
                }
            }
            
        }
    }
}