/*
*  	Protocoale de comunicatii: 
*  	Laborator 6: UDP
*	client mini-server de backup fisiere
*/

#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h> 

#include "helpers.h"


void usage(char*file)
{
	fprintf(stderr,"Usage: %s ip_server port_server file\n",file);
	exit(0);
}

int get_size(int fd) {
	int old = lseek(fd, 0, SEEK_CUR);
	int size = lseek(fd, 0, SEEK_END);
	lseek(fd, old, SEEK_SET);

	return size;
}

/*
*	Utilizare: ./client ip_server port_server nume_fisier_trimis
*/
int main(int argc,char**argv)
{
	if (argc!=4)
		usage(argv[0]);
	
	int fd, s;
	struct sockaddr_in to_station;
	char buf[BUFLEN];


	/*Deschidere socket*/
	int sock = socket(AF_INET, SOCK_DGRAM, 0);
	DIE(sock < 0, "Eroare la crearea socket-ului");
	
	/* Deschidere fisier pentru citire */
	DIE((fd=open(argv[3],O_RDONLY)) < 0,"open file");
	
	/*Setare struct sockaddr_in pentru a specifica unde trimit datele*/
	
	to_station.sin_family = AF_INET;
	to_station.sin_port = htons(atoi(argv[2]));
	inet_aton(argv[1], &to_station.sin_addr);
	
	/*
	*  cat_timp  mai_pot_citi
	*		citeste din fisier
	*		trimite pe socket
	*/	

	sprintf(buf, "%s", argv[3]);
	sendto(sock, buf, BUFLEN, 0, (struct sockaddr *)&to_station, sizeof(to_station));
	memset(buf, 0, BUFLEN);

	/* Trimitere date */

	while (1) {
		int r = read(fd, buf, BUFLEN);
		DIE(r < 0, "Eroare la citire");

		if (!r) {
			break;
		}

		s = sendto(sock, buf, s, 0, (struct sockaddr *)&to_station, sizeof(to_station));
		DIE(s < 0, "Eroare la trimiterea datelor");

		memset(buf, 0, BUFLEN);
	}

	memset(buf, 0, BUFLEN);
	sprintf(buf, "EXIT");

	s = sendto(sock, buf, strlen("EXIT") + 1, 0, (struct sockaddr *)&to_station, sizeof(to_station));
	DIE(s < 0, "Eroare la trimiterea datelor");

	/*Inchidere socket*/
	close(sock);
	
	/*Inchidere fisier*/
	close(fd);
	
	return 0;
}
