#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <vector>

#include "helpers.h"

using namespace std;

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_address server_port\n", file);
    exit(0);
}

int main(int argc, char *argv[]) {
    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    fd_set set;
    fd_set copy;

    FD_ZERO(&set);
    FD_ZERO(&copy);

    if (argc < 3) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    ret = inet_aton(argv[1], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    FD_SET(sockfd, &set);
    FD_SET(0, &set);

    while (1) {
        copy = set;
        int sel = select(sockfd + 1, &copy, NULL, NULL, NULL);
        DIE(sel < 0, "select");

        if (FD_ISSET(0, &copy)) {
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);

            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            } else {
                printf("Asteapta raspuns de la server!");
            }
        }

        if (FD_ISSET(sockfd, &copy)) {
            // se primeste mesaj de la server
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd, buffer, sizeof(buffer), 0);
            DIE(n < 0, "recv");

            printf("%s", buffer);
            break;
        }
    }

    close(sockfd);

    return 0;
}
