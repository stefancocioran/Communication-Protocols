#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(void)
{
	msg r;
	my_payload rec_message;

	int i, res;
	int OK = 0, CORRUPT = 0;

	printf("[RECEIVER] Starting.\n");
	init(HOST, PORT);
	
	for (i = 0; i < COUNT; i++) {
		
		/* wait for message */
		res = recv_message(&r);
		if (res < 0) {
			perror("[RECEIVER] Receive error. Exiting.\n");
			return -1;
		}
		
		rec_message = *((my_payload*) r.payload);
		//printf("%s\n", rec_message.payload);

		if (rec_message.parity == message_parity(&rec_message)) {
			OK++;
		} else {
			CORRUPT++;
		}

		/* send dummy ACK */
		memcpy(r.payload, "ACK", strlen("ACK") + 1);
		
		res = send_message(&r);
		if (res < 0) {
			perror("[RECEIVER] Send ACK error. Exiting.\n");
			return -1;
		}
	}

	printf("[RECEIVER] OK = %d; CORRUPT = %d => %.2lf%% success rate\n", OK, CORRUPT, 1.0 * OK * 100 / (OK + CORRUPT));
	printf("[RECEIVER] Finished receiving..\n");
	
	return 0;
}
