#include "server_common.hpp"
#include <poll.h>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h> // for read and write

using namespace std;

class Player
{
    // private:
    //     string question = "Podaj imie: ";
    public:
        string name;
        unsigned int room_id;
        int fd;
        pollfd pfd;
        bool nameAsked = false;

        void askName()
        {
            char buf[64];
            //write(this->fd, question.c_str(), question.length());
            ssize_t bytes_read = read(this->fd, buf, sizeof(buf) - 1);
            if (bytes_read > 0)
            {
                buf[bytes_read] = '\0'; // Null-terminate the string
                this->name = buf;
                this->nameAsked = true;
            }
        }
};

int main(int argc, char **argv)
{
    int servSock = socket_bind_listen(argc, argv, SOCK_STREAM);
    char a[] = "Witaj graczu! Podaj imie: \n";

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
            write(player_fd, a, sizeof(a));
            Player newPlayer;
            newPlayer.fd = player_fd;
            newPlayer.pfd = {player_fd, POLLIN, 0};
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
                }
                else
                {
                    char buffer[128]{};
                    ssize_t bytes_read = read(players[i].fd, buffer, sizeof(buffer));
                    if (bytes_read <= 0)
                    {
                        if (bytes_read == 0)
                        {
                            printf("Gracz %s opuscil gre.\n", players[i].name.c_str());
                        }
                        close(players[i].fd);
                        players.erase(players.begin() + i);
                        --i;
                    }
                    else
                    {
                        buffer[bytes_read] = '\0'; // Null-terminate the string
                        printf("Wiadomosc od %s: %s\n", players[i].name.c_str(), buffer);
                    }
                }
            }
        }
    }
}