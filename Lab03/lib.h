#ifndef LIB
#define LIB

#define MSGSIZE		1400
#define COUNT		100

typedef struct {
  int len;
  char payload[MSGSIZE];
} msg;

typedef struct {
    int parity; // 0 sau 1
    char payload[MSGSIZE - sizeof(int)];
} my_payload;

int byte_parity(unsigned char c) {
  
	unsigned char mask = 1 << 7;
	int i, result = (c & mask) != 0;
	mask >>= 1;

	for (i = 0; i < 7; i++) {
	  result ^= (c & mask) != 0;
		mask >>= 1;
	}

	return result;
}

int message_parity(const my_payload *message) {

	int result = 0;

	for (int i = 0; i < strlen(message->payload); i++) {
		result ^= byte_parity(message->payload[i]);
	}
	
	return result;
}

void init(char* remote,int remote_port);
void set_local_port(int port);
void set_remote(char* ip, int port);
int send_message(const msg* m);
int recv_message(msg* r);

#endif

