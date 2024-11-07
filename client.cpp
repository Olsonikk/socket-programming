#include "client_common.hpp"

int main(int argc, char **argv)
{
    int sockfd = getaddrinfo_socket_connect(argc, argv);
    char a[32];
    read(sockfd, a, 32);
    if(strcmp(a, "START") == 0)
    {
        printf("podaj ruch: ");
        scanf("%31[^\n]", a);
    
        if (strcmp(a, "forward") == 0) {
            printf("Ruch to 'forward'\n");
            write(sockfd, "0", 1);
        }
        else if (strcmp(a, "backward") == 0) {
            printf("Ruch to 'backward'\n");
            write(sockfd, "1", 1);
        }
        else if (strcmp(a, "left") == 0) {
            printf("Ruch to 'left'\n");
            write(sockfd, "2", 1);

        }
        else if (strcmp(a, "right") == 0) {
            printf("Ruch to 'right'\n");
            write(sockfd, "3", 1);
        }
        else {
            printf("Nieznany ruch: %s\n", a);
        }
        char buf[10];
        int res = read(sockfd, buf, 10);
        write(1, buf, res);
    }
    

    close(sockfd);
    return 0;
}