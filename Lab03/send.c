#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000


int main(int argc, char *argv[])
{
	msg t;
	my_payload message;

	int i, res;

	printf("[SENDER] Starting.\n");
	init(HOST, PORT);

	printf("[SENDER]: BDP = %d\n", atoi(argv[1]));

	/* compute W */

	int w = atoi(argv[1]) * 1000 / (sizeof(msg) * 8);
	
	/* ---BONUS---
	Atunci cand se trimite o cantitate de date care depaseste valoarea WINDOW_SIZE,
	comunicarea poate fi afectata. Primele W cadre sunt primite, iar restul s-ar pierde.
	Receiver-ul nu trimite ACK pentru pachetele pierdute, ci ar trebui sa trimita un NACK.
	Daca nu limitam pe W la COUNT, am observat ca executia programului nu se opreste,
	nu functioneaza cum ar trebui.
	*/
	
	if (w > COUNT) {
		w = COUNT;
	}

	printf("[SENDER]: w = %d\n", w);

	/* msg to send */

	memcpy(message.payload, "payload", strlen("payload") + 1);
	message.parity = message_parity(&message);
	
	t.len = sizeof(int) + strlen("payload") + 1;
	memcpy(t.payload, &message, t.len);

	/* sending w frames */

	for (int i = 0; i < w; i++) {
		res = send_message(&t);
		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}

	for (i = 0; i < COUNT - w; i++) {

		/* wait ACK for previous frame */
		res = recv_message(&t);

		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		} 

		/* send current frame */

		memset(&t, 0, sizeof(msg));
		memcpy(message.payload, "payload", strlen("payload") + 1);
		message.parity = message_parity(&message);

		t.len = sizeof(int) + strlen("payload") + 1;
		memcpy(t.payload, &message, t.len);

		res = send_message(&t);

		if (res < 0) {
			perror("[SENDER] Send error. Exiting.\n");
			return -1;
		}
	}

	/* waiting for the rest of the confirmations */

	for (int i = 0; i < w; i++) {
		/* wait for ACK */
		
		res = recv_message(&t);
		if (res < 0) {
			perror("[SENDER] Receive error. Exiting.\n");
			return -1;
		}
	}

	printf("[SENDER] Job done, all sent.\n");

	return 0;
}
