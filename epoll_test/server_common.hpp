#pragma once
#include <cstdio>
#include <cstdlib>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int socket_bind_listen(int argc, char **argv, int sock_type) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    sockaddr_in localAddress{.sin_family = AF_INET,
                             .sin_port = htons(atoi(argv[1])),
                             .sin_addr = {htonl(INADDR_ANY)}};

    int servSock = socket(PF_INET, sock_type, 0);

    if (sock_type == SOCK_STREAM) {
        const int one = 1;
        setsockopt(servSock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    }

    if (bind(servSock, (sockaddr *)&localAddress, sizeof(localAddress))) {
        perror("Bind failed!");
        exit(1);
    }

    if (sock_type == SOCK_STREAM)
        listen(servSock, 1);

    return servSock;
}
