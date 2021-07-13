/*
 *  	Protocoale de comunicatii:
 *  	Laborator 6: UDP
 *	client mini-server de backup fisiere
 */

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "helpers.h"

void usage(char *file) {
    fprintf(stderr, "Usage: %s ip_server port_server file\n", file);
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 4) usage(argv[0]);

    int s;
    struct sockaddr_in to_station;
    char buf[BUFLEN];

    /*Deschidere socket*/
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(sock < 0, "Eroare la crearea socket-ului");

    int number = atoi(argv[3]);

    /*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
    to_station.sin_family = AF_INET;
    to_station.sin_port = htons(atoi(argv[2]));
    inet_aton(argv[1], &to_station.sin_addr);

    sprintf(buf, "%s", argv[3]);
    s = sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&to_station,
               sizeof(to_station));
    DIE(s < 0, "Eroare la trimiterea datelor");

    printf("Numarul %d a fost trims cu succes catre server!\n", number);

    /*Inchidere socket*/
    close(sock);

    return 0;
}
