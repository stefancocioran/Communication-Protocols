/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h> 

#include "helpers.h"


void usage(char*file)
{
	fprintf(stderr,"Usage: %s server_port file\n",file);
	exit(0);
}

/*
*	Utilizare: ./server server_port nume_fisier
*/
int main(int argc,char**argv)
{
	if (argc!=3)
		usage(argv[0]);
	
	struct sockaddr_in my_sockaddr, from_station ;
	char buf[BUFLEN];

	/*Deschidere socket*/
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock < 0, "Eroare la crearea socket-ului");

	/*Setare struct sockaddr_in pentru a asculta pe portul respectiv */
	my_sockaddr.sin_family = AF_INET;
	my_sockaddr.sin_port = htons(atoi(argv[1]));
	my_sockaddr.sin_addr.s_addr = INADDR_ANY;

	
	/* Legare proprietati de socket */
	DIE(bind(sock, (const struct sockaddr *)&my_sockaddr, sizeof(my_sockaddr)) < 0 , "Eroare la legarea socket-ului");

	/* Deschidere fisier pentru scriere */
	int fd;
	DIE((fd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0644))==-1,"open file");
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din socket
	*		pune in fisier
	*/

    socklen_t socklen = sizeof(from_station);
	int r = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from_station, &socklen);
	DIE(r < 0, "Eroare la primirea datelor");

    printf("S-a primit de la clientul cu adresa %s si portul %d fisierul cu numele %s\n", inet_ntoa(from_station.sin_addr), ntohs(from_station.sin_port), buf);

    fd = open(buf, O_WRONLY | O_CREAT, 0644);
    memset(buf, 0, BUFLEN);

	/* Primire date */

    while (1) {
		r = recvfrom(sock, buf, BUFLEN, 0, (struct sockaddr *) &from_station, &socklen);
		DIE(r < 0, "Eroare la primirea datelor");
        
		if (strcmp(buf, "EXIT") == 0) {
            break;
        }

        int w = write(fd, buf, BUFLEN);
		DIE(w < 0, "Eroare la scriere");

        memset(buf, 0, BUFLEN);
    }
	
	/*Inchidere socket*/	
	close(sock);
	
	/*Inchidere fisier*/
	close(fd);

	return 0;
}
