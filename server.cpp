#include "server_common.hpp"
#include <poll.h>

#define MAX_PLAYERS 10
int players_num = 1;




void printdesclist(pollfd *pfds, int n)
{
    for(int i=0; i<n; i++)
    {
        printf("fd: %d, events: %d, revents: %d\n", pfds[i].fd, pfds[i].events, pfds[i].revents);
    }
}

void remove_player(pollfd *pfds, int index, int &players_num)
{
    close(pfds[index].fd);
    pfds[index].fd = -1;
    for (int i = index; i < players_num - 1; i++)
    {
        pfds[i] = pfds[i + 1];
    }
    pfds[players_num - 1].fd = -1;
    players_num--;
}

int main(int argc, char **argv)
{
    int servSock = socket_bind_listen(argc, argv, SOCK_STREAM);
    //int player1, player2;
    char a[] = "Witaj graczu! \n";
    char b[] = "START";
    //player1 = accept(servSock, nullptr, nullptr);
    //send(player1, a, sizeof(a), 0);
    //player2 = accept(servSock, nullptr, nullptr);
    // send(player1, b, sizeof(b), 0);
    // send(player2, b, sizeof(b), 0);
    // close(servSock);

    pollfd pfds[MAX_PLAYERS]{};
    pfds[0].fd = servSock;
    pfds[0].events = POLLIN;
    for(int i=1; i<MAX_PLAYERS; i++)
    {
        pfds[i].fd = -1;
    }

    while(1)
    {
        sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int ile_gotowych = poll(pfds, players_num, -1);
        if (ile_gotowych == -1)
        {
            perror("problem z poll.");
            exit(1);
        }
        
        if(ile_gotowych > 0)
        {
            if(pfds[0].revents & POLLIN)
            {
                int player_fd = accept(servSock, (sockaddr*)&client_addr, &client_addr_size);
                if(player_fd == -1)
                {
                    perror("problem z accept.");
                    exit(1);
                }
                printf("Gracz %d dolaczyl do gry.\n", player_fd - 3);

                for(int i=1; i<MAX_PLAYERS; i++)
                {
                    if(pfds[i].fd == -1)
                    {
                        pfds[i].fd = player_fd;
                        pfds[i].events = POLLIN;
                        break;
                    }
                }
                players_num++;

                write(player_fd, a, sizeof(a)); 
            }

            for(int i=1; i<players_num; i++)
            {
                  if (pfds[i].fd > 0 && pfds[i].revents & POLLIN)
                  {
                    char buffer[128];
                    int bytes_read = read(pfds[i].fd, buffer, sizeof(buffer));

                    if(bytes_read == 0)
                    {
                        printf("Gracz %d opuscil gre.\n", pfds[i].fd - 3);
                        remove_player(pfds, i, players_num);
                        //printdesclist(pfds, MAX_PLAYERS);
                        continue;
                    }
                    printf("Wiadomosc od gracza %d: %s \n",i,buffer);
                  }
            }
        }
    }

}

