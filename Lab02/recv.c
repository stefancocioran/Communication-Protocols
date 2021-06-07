#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10001
#define SIZE 20

int main(int argc,char** argv) {
  
	msg r;

  	init(HOST,PORT);

  	if (recv_message(&r) < 0) {
    	perror("Receive message");
    	return -1;
  	}

  	printf("[recv] Received filename: <%s>\n", r.payload);
  	printf("[recv] Creating copy...\n");

  	char filename[SIZE];
  	strcpy(filename, r.payload);
  	filename[strlen(filename) - 4] = '\0';
  	strcat(filename, "_copy.txt");
  	
  	int fd = open(filename, O_CREAT | O_WRONLY, 0755);
  	
  	if (fd < 0) {
		perror("Can't open file.");
		return -1;  
	}
		
  	sprintf(r.payload,"%s", "File copy created");
  	r.len = strlen(r.payload) + 1;
  	
  	send_message(&r);
  	printf("[recv] ACK sent\n");

 	recv_message(&r);
	printf("[recv] Got msg with filesize: <%s bytes>\n", r.payload);
	int filesize = atoi(r.payload);

	sprintf(r.payload,"%s", "File size received");
  	r.len = strlen(r.payload) + 1;
  	
  	send_message(&r);
  	printf("[recv] ACK sent\n");

  	int count = 1;

  	while (filesize > 0) {
  		
  		recv_message(&r);
  		printf("[recv] Got msg with the following content: \n%s\n", r.payload);

		printf("[recv] Writing package %d in file copy...\n", count);

  		int w = write(fd, &r.payload, strlen(r.payload));
  		
  		if (w < 0) {
			perror("Write error");
			return -1;
		}
			
  		filesize -= w;

  		sprintf(r.payload,"%s %d %s", "Package", count, "received");
  		r.len = strlen(r.payload) + 1;
  		count++;
  	
  		send_message(&r);
  		printf("[recv] ACK sent\n");
  	}  	

  	close(fd);
 
  return 0;
}
