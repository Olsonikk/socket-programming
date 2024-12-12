#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <arpa/inet.h>

const int MAX_CLIENTS = 5;

ssize_t readData(int fd, char *buffer, ssize_t buffsize) {
    auto ret = read(fd, buffer, buffsize);
    if (ret == -1) error(1, errno, "read failed on descriptor %d", fd);
    return ret;
}

void writeData(int fd, char *buffer, ssize_t count) {
    auto ret = write(fd, buffer, count);
    if (ret == -1) error(1, errno, "write failed on descriptor %d", fd);
    if (ret != count) error(0, errno, "wrote less than requested to descriptor %d (%ld/%ld)", fd, count, ret);
}

int main(int argc, char **argv) {
    if (argc != 2) error(1, 0, "Usage: %s <port>", argv[0]);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[1]));
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    if (bind(serv_sock, (sockaddr *)&server_addr, sizeof(server_addr)) != 0) {
        perror("bind");
        close(serv_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(serv_sock, MAX_CLIENTS) != 0) {
        perror("listen");
        close(serv_sock);
        exit(EXIT_FAILURE);
    }

    pollfd desc[MAX_CLIENTS + 1];
    desc[0].fd = serv_sock;
    desc[0].events = POLLIN;

    int descCounter = 1;

    while (true) {
        int ready = poll(desc, descCounter, -1);
        if (ready == -1) {
            perror("poll");
            close(serv_sock);
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < descCounter; ++i) {
            if (desc[i].revents & (POLLERR | POLLHUP | POLLRDHUP)) {
                printf("Client disconnected (fd: %d)\n", desc[i].fd);
                close(desc[i].fd);
                desc[i] = desc[descCounter - 1];
                descCounter--;
                i--; // Check the new descriptor at this index
                continue;
            }

            if ((desc[i].revents & POLLIN) && desc[i].fd == serv_sock) {
                sockaddr_in clientAddr{};
                socklen_t clientAddrSize = sizeof(clientAddr);
                int clientFd = accept(serv_sock, (sockaddr *)&clientAddr, &clientAddrSize);

                if (clientFd == -1) {
                    perror("accept");
                    continue;
                }

                if (descCounter < MAX_CLIENTS + 1) {
                    desc[descCounter].fd = clientFd;
                    desc[descCounter].events = POLLIN;
                    descCounter++;
                    printf("New connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
                } else {
                    printf("Max clients reached, rejecting connection from: %s:%hu\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
                    close(clientFd);
                }
                break;
            } else if ((desc[i].revents & POLLIN) && desc[i].fd != serv_sock) {
                char buffer[128];
                ssize_t read_len = readData(desc[i].fd, buffer, sizeof(buffer) - 1);
                if (read_len > 0) {
                    buffer[read_len] = '\0';
                    printf("Received from fd %d: %s\n", desc[i].fd, buffer);
                }
            }
            ready--;
        }
    }
}
