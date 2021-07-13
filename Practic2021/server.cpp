#include <arpa/inet.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "helpers.h"

using namespace std;

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_port\n", file);
    exit(0);
}

string compute_payload(vector<int> numbers_received) {
    string payload;
    float average;
    int count = 0;
    payload += "{ ";
    for (auto num : numbers_received) {
        payload += to_string(num) + " ";
        average += num;
        count++;
    }
    payload += "}\nMedia: " + to_string(average / count) + "\n";

    return payload;
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int tcp_sock, udp_sock, newsockfd, portno;
    int n, i, ret;
    char buffer[BUFLEN];
    struct sockaddr_in serv_addr_tcp, serv_addr_udp, cli_addr;
    socklen_t clilen;

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

    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    DIE(tcp_sock < 0, "socket");

    /*Deschidere socket*/
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(udp_sock < 0, "socket_udp");

    portno = atoi(argv[1]);
    DIE(portno == 0, "atoi");

    memset((char *)&serv_addr_tcp, 0, sizeof(serv_addr_tcp));
    serv_addr_tcp.sin_family = AF_INET;
    serv_addr_tcp.sin_port = htons(portno);
    serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;

    memset((char *)&serv_addr_udp, 0, sizeof(serv_addr_udp));
    serv_addr_udp.sin_family = AF_INET;
    serv_addr_udp.sin_port = htons(portno);
    serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

    ret = bind(tcp_sock, (struct sockaddr *)&serv_addr_tcp,
               sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    ret = bind(udp_sock, (struct sockaddr *)&serv_addr_udp,
               sizeof(struct sockaddr));
    DIE(ret < 0, "bind");

    ret = listen(tcp_sock, MAX_CLIENTS);
    DIE(ret < 0, "listen");

    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea read_fds
    FD_SET(tcp_sock, &read_fds);
    FD_SET(udp_sock, &read_fds);
    fdmax = max(tcp_sock, udp_sock);

    // se adauga stdin in multime
    FD_SET(STDIN_FILENO, &read_fds);

    vector<int> numbers_received;

    bool forever = true;
    while (forever) {
        tmp_fds = read_fds;

        ret = select(fdmax + 1, &tmp_fds, NULL, NULL, NULL);
        DIE(ret < 0, "select");

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &tmp_fds)) {
                if (i == tcp_sock) {
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu
                    // listen), pe care serverul o accepta
                    clilen = sizeof(cli_addr);
                    newsockfd =
                        accept(tcp_sock, (struct sockaddr *)&cli_addr, &clilen);
                    DIE(newsockfd < 0, "accept");

                    // se adauga noul socket intors de accept() la multimea
                    // descriptorilor de citire
                    FD_SET(newsockfd, &read_fds);

                    fdmax = max(newsockfd, fdmax);

                    memset(buffer, 0, BUFLEN);
                    if (numbers_received.empty()) {
                        sprintf(buffer, "Lista este vida.\n");
                    } else {
                        string payload = compute_payload(numbers_received);
                        sprintf(buffer, "%s", payload.c_str());
                    }

                    n = send(newsockfd, buffer, strlen(buffer) + 1, 0);
                    DIE(n < 0, "send");

                } else if (i == STDIN_FILENO) {
                    // daca serverul primeste comanda "exit", se inchid
                    // conexiunile cu toti clientii se inchide serverul

                    memset(buffer, 0, BUFLEN);
                    fgets(buffer, BUFLEN - 1, stdin);

                    if (strncmp(buffer, "exit", 4) == 0) {
                        // se iese din bucla "while" si se opreste serverul
                        forever = false;
                        break;
                    } else {
                        fprintf(stderr,
                                "EROARE: Singura comanda acceptata este "
                                "\"exit\".\n");
                    }
                } else if (i == udp_sock) {
                    // s-au primit date pe unul din socketii de client udp,
                    // asa ca serverul trebuie sa le receptioneze

                    memset(buffer, 0, BUFLEN);
                    n = recv(i, buffer, BUFLEN, 0);
                    DIE(n < 0, "recv");

                    int number;
                    sscanf(buffer, "%d", &number);
                    numbers_received.push_back(number);
                    printf("Am primit %d de la %s:%d\n", number,
                           inet_ntoa(serv_addr_udp.sin_addr),
                           serv_addr_udp.sin_port);

                } else {
                    // s-au primit date pe unul din socketii de client tcp,
                    // asa ca serverul trebuie sa le receptioneze
                    memset(buffer, 0, BUFLEN);
                    n = recv(i, buffer, sizeof(buffer), 0);
                    DIE(n < 0, "recv");

                    // serverul nu va primi niciodata date de la clienti TCP
                    if (n == 0) {
                        // conexiunea s-a inchis
                        // se inchide socket-ul si este scos din multimea de
                        close(i);
                        FD_CLR(i, &read_fds);
                    }
                }
            }
        }
    }

    close(tcp_sock);
    close(udp_sock);

    return 0;
}
