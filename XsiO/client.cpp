#include <arpa/inet.h>
#include <netdb.h>
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
#define SIZE 3

void usage(char *file) {
    fprintf(stderr, "Usage: %s server_address server_port\n", file);
    exit(0);
}

int **new_game() {
    int **board;
    board = new int *[SIZE];

    for (int i = 0; i < SIZE; i++) {
        board[i] = new int[SIZE];
    }

    // Initialize all table values with -1
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = 0;
        }
    }
    return board;
}

void print_board(int **board) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == -1) {
                printf("O");
            } else if (board[i][j] == 1) {
                printf("X");
            } else {
                printf("*");
            }
        }
        printf("\n");
    }
    printf("\n");
}

bool check_valid(int **board, int i, int j) {
    if (board[i][j] != 0) {
        printf("Celula este ocupata! Alege alta.\n");
        return false;
    }
    return true;
}

bool check_win(int **board, int XorO) {
    int count = 0;
    for (int i = 0; i < SIZE; i++) {
        count = 0;
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == XorO) count++;
        }
        if (count == SIZE) return true;
    }

    for (int i = 0; i < SIZE; i++) {
        count = 0;
        for (int j = 0; j < SIZE; j++) {
            if (board[j][i] == XorO) count++;
        }
        if (count == SIZE) return true;
    }

    count = 0;
    for (int i = 0; i < SIZE; i++) {
        if (board[i][i] == XorO) count++;
    }
    if (count == SIZE) return true;

    return false;
}

bool board_full(int **board) {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == 0) {
                return false;
            }
        }
    }
    return true;
}

int main(int argc, char *argv[]) {
    int sockfd, n, ret;
    struct sockaddr_in serv_addr;
    char buffer[BUFLEN];
    fd_set set;
    fd_set copy;

    FD_ZERO(&set);
    FD_ZERO(&copy);

    if (argc < 3) {
        usage(argv[0]);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(atoi(argv[2]));
    ret = inet_aton(argv[1], &serv_addr.sin_addr);
    DIE(ret == 0, "inet_aton");

    ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(ret < 0, "connect");

    FD_SET(sockfd, &set);
    FD_SET(0, &set);

    // BOARD

    int **board = new_game();
    // -1 pt zero si 1 pt X
    int XorO;
    bool my_turn = false;
    bool game_started = false;

    while (1) {
        copy = set;
        int sel = select(sockfd + 1, &copy, NULL, NULL, NULL);
        DIE(sel < 0, "select");

        if (FD_ISSET(0, &copy)) {
            memset(buffer, 0, BUFLEN);
            fgets(buffer, BUFLEN - 1, stdin);

            if (strncmp(buffer, "exit", 4) == 0) {
                printf("Ai abandonat jocul! Oponentul castiga.\n");
                break;
            }

            int i, j;
            sscanf(buffer, "%d %d", &i, &j);

            if (!game_started) {
                printf("Jocul nu a inceput! Se cauta oponent.\n");
                continue;
            }

            // inainte de toate sa verific daca mai sunt celule goale

            if (!check_valid(board, i, j)) {
                continue;
            }
            if (!my_turn) {
                printf("Nu e randul tau! Asteapta.\n");
                continue;
            }

            my_turn = false;
            board[i][j] = XorO;

            // se trimite mesaj la server
            n = send(sockfd, buffer, strlen(buffer), 0);
            DIE(n < 0, "send");
            print_board(board);

            if (check_win(board, XorO)) {
                printf("Ai castigat!\n");
                break;
            }

            if (board_full(board)) {
                printf("Remiza!\n");
                break;
            }
        }

        if (FD_ISSET(sockfd, &copy)) {
            // se primeste mesaj de la server
            memset(buffer, 0, BUFLEN);
            n = recv(sockfd, buffer, sizeof(buffer), 0);

            if (strncmp(buffer, "exit", 4) == 0) {
                break;
            } else if (strncmp(buffer, "win", 3) == 0) {
                printf("Oponentul a caștigat.\n");
                break;
            } else if (strncmp(buffer, "resign", 6) == 0) {
                printf("Oponentul a abandonat meciul. Ai castigat!\n");
                break;
            } else if (strncmp(buffer, "draw", 4) == 0) {
                printf("Remiza!\n");
                break;
            } else if (strncmp(buffer, "X", 1) == 0) {
                printf("Joci cu X!\n");
                game_started = true;
                my_turn = true;
                XorO = 1;
            } else if (strncmp(buffer, "O", 1) == 0) {
                printf("Joci cu O!\n");
                game_started = true;
                XorO = -1;
            } else {
                int i, j;
                sscanf(buffer, "%d %d", &i, &j);
                board[i][j] = -1 * XorO;
                print_board(board);

                if (check_win(board, -1 * XorO)) {
                    printf("Oponentul a castigat.\n");
                    break;
                }
                my_turn = true;

                if (board_full(board)) {
                    printf("Remiza!\n");
                    break;
                }
            }
        }
    }

    close(sockfd);

    return 0;
}
