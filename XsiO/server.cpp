#include <arpa/inet.h>
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
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

int search_pair(vector<pair<int, int>> &perechi, int pereche) {
    vector<pair<int, int>>::iterator it;
    it = perechi.begin();

    for (auto it = perechi.begin(); it != perechi.end(); ++it) {
        if (it->first == pereche) {
            return it->second;
        } else if (it->second == pereche) {
            return it->first;
        }
    }
    return -1;
}

void remove_pair(vector<pair<int, int>> &perechi, int pereche) {
    vector<pair<int, int>>::iterator it;
    it = perechi.begin();

    for (auto it = perechi.begin(); it != perechi.end(); ++it) {
        if (it->first == pereche || it->second == pereche) {
            perechi.erase(it);
            return;
        }
    }
}


int main(int argc, char *argv[]) {
    int sockfd, newsockfd, portno;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr, cli_addr;
    int n, i, ret;
    socklen_t clilen;

    vector<pair<int, int>> perechi;
    int client1 = -1, client2 = -1;

    fd_set read_fds;  // multimea de citire folosita in select()
    fd_set tmp_fds;   // multime folosita temporar
    int fdmax;        // valoare maxima fd din multimea read_fds

    if (argc < 2) {
        usage(argv[0]);
    }

    // se goleste multimea de descriptori de citire (read_fds) si multimea
    // temporara (tmp_fds)
    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    ret = bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    ret = listen(sockfd, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea read_fds
    FD_SET(sockfd, &read_fds);
    fdmax = sockfd;

    // adaug stdin in multime
    FD_SET(STDIN_FILENO, &read_fds);

    int forever = 1;
    while (forever) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == sockfd) {
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu
                    // listen), pe care serverul o accepta
                    clilen = sizeof(cli_addr);
                    newsockfd =
                        accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");

                    // se adauga noul socket intors de accept() la multimea
                    // descriptorilor de citire
                    FD_SET(newsockfd, &read_fds);
                    if (newsockfd > fdmax) {
                        fdmax = newsockfd;
                    }

                    if (client1 != -1) {
                        client2 = newsockfd;
                        perechi.push_back(make_pair(client1, client2));

                        n = send(client1, "X", strlen("X") + 1, 0);
                        DIE(n == -1, "send()");

                        n = send(client2, "O", strlen("X") + 1, 0);
                        DIE(n == -1, "send()");

                        // reset
                        client1 = -1;
                        client2 = -1;
                    } else {
                        client1 = newsockfd;
                    }

                } else if (i == STDIN_FILENO) {
                    // daca serverul primeste comanda "exit", se inchid
                    // conexiunile cu toti clientii se inchide serverul

                    char buffer[BUFLEN];
                    fgets(buffer, BUFLEN - 1, stdin);
                    // printf("serverul primeste comanda: %s\n", buffer);

                    if (strncmp(buffer, "exit", 4) == 0) {
                        for (int j = 0; j <= fdmax; j++) {
                            if (FD_ISSET(j, &read_fds) && j != STDIN_FILENO &&
                                j != sockfd) {
                                n = send(j, buffer, strlen(buffer) + 1, 0);
                                DIE(n == -1, "send");
                                close(j);
                            }
                        }
                        forever = 0;
                        break;
                    }
                } else {
                    // s-au primit date pe unul din socketii de client,
                    // asa ca serverul trebuie sa le receptioneze
                    memset(buffer, 0, BUFLEN);
                    n = recv(i, buffer, sizeof(buffer), 0);
                    DIE(n < 0, "recv");

                    if (n == 0) {
                        // conexiunea s-a inchis
                        close(i);

                        int pair = search_pair(perechi, i);
                        if (pair == -1) {
                        }
                        printf(
                            "player_%d a abandonat meciul. Castigatorul "
                            "este player_%d.\n",
                            i, pair);
                        remove_pair(perechi, i);

                        memset(buffer, 0, BUFLEN);
                        sprintf(buffer, "resign");
                        n = send(pair, buffer, strlen(buffer) + 1, 0);
                        DIE(n < 0, "send");
                        close(pair);

                        // se scoate din multimea de citire socketurile inchise
                        FD_CLR(i, &read_fds);
                        FD_CLR(pair, &read_fds);

                    } else {
                        int sock_dest;
                        int pair = search_pair(perechi, i);
                        if (strncmp(buffer, "win", 3) == 0) {
                            printf(
                                "player_%d a castigat meciul vs. player_"
                                "%d.\n",
                                i, pair);
                            remove_pair(perechi, i);
                            n = send(pair, buffer, strlen(buffer) + 1, 0);
                            DIE(n < 0, "send");
                        } else if (strncmp(buffer, "draw", 4) == 0) {
                            printf("Remiza intre player_%d si player_%d.\n", i,
                                   pair);
                            remove_pair(perechi, i);
                            n = send(pair, buffer, strlen(buffer) + 1, 0);
                            DIE(n < 0, "send");

                        } else {
                            // se fac mutari
                            n = send(pair, buffer, strlen(buffer) + 1, 0);
                            DIE(n < 0, "send");
                        }
                    }
                }
            }
        }
    }

    close(sockfd);

    return 0;
}
