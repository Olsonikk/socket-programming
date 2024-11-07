#include "server_common.hpp"
#include <poll.h>

int main(int argc, char **argv)
{
    int servSock = socket_bind_listen(argc, argv, SOCK_STREAM);
    int player1, player2;
    char a[] = "Oczekiwanie na drugiego gracza\n";
    char b[] = "START";
    player1 = accept(servSock, nullptr, nullptr);
    //send(player1, a, sizeof(a), 0);
    player2 = accept(servSock, nullptr, nullptr);
    send(player1, b, sizeof(b), 0);
    send(player2, b, sizeof(b), 0);
    close(servSock);

    pollfd pfds[2]{};
    pfds[0].fd = player1;
    pfds[1].fd = player2;
    pfds[0].events = POLLIN;
    pfds[1].events = POLLIN;

    while(1)
    {
        int ile_gotowych = poll(pfds, 2, -1);
        
        if(pfds[0].revents & POLLIN)
        {
            char out[10];
            int len_read = read(player1, out, 10);
            write(player2, out, len_read);
        } 
        if(pfds[1].revents & POLLIN)
        {
            char out[10];
            int len_read = read(player2, out, 10);
            write(player1, out, len_read);
        } 

    }

    shutdown(player1, SHUT_RDWR);
    close(player1);
    shutdown(player2, SHUT_RDWR);
    close(player2);

}