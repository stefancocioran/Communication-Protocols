#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "link_emulator/lib.h"

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc,char** argv){
	
	init(HOST,PORT);

	msg message;
	
	int fd = open("file.txt", O_RDONLY);

	printf("[send] Sending filename...\n");
	sprintf(message.payload,"%s", "file.txt");
	message.len = strlen(message.payload) + 1;
	send_message(&message);


	if (recv_message(&message) < 0){
		perror("Receive error ...");
		return -1;
	}
	else {
		printf("[send] Got reply with payload: %s\n", message.payload);
	}

	printf("%s\n", "[send] Computing file size...");
	
	int filesize = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	printf("%s\n", "[send] Sending file size...");

	sprintf(message.payload,"%d", filesize);
	message.len = strlen(message.payload) + 1;
	send_message(&message);

	if (recv_message(&message) < 0){
		perror("Receive error ...");
		return -1;
	} else {
		printf("[send] Got reply with payload: %s\n", message.payload);
	}

	while (filesize > 0) {	
		
		memset(&message, 0, sizeof(message));
		int r = read(fd, message.payload, sizeof(message.payload));
		
		if (r < 0) {
			perror("Read error");
			return -1;
		}
		
		printf("%s\n", "[send] Sending package...");
		message.len = strlen(message.payload) + 1;
		send_message(&message);

		if (recv_message(&message) < 0){
			perror("Receive error ...");
			return -1;
		} else {
			printf("[send] Got reply with payload: %s\n", message.payload);
		}

		filesize -= r;
	}

	close(fd);
	
	return 0;
}
