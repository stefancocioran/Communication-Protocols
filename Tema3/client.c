#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

#include "helpers.h"
#include "requests.h"

int main(int argc, char *argv[]) {
    char *login_cookie = NULL;
    char *token = NULL;
    char buffer[BUFLEN];

    while (1) {
        memset(buffer, 0, BUFLEN);
        fgets(buffer, BUFLEN - 1, stdin);

        if (strcmp(buffer, REGISTER) == 0) {
            register_login("/api/v1/tema/auth/register");
        } else if (strcmp(buffer, LOGIN) == 0) {
            if (login_cookie == NULL) {
                login_cookie = register_login("/api/v1/tema/auth/login");
            } else {
                printf("User already logged in!\n");
            }
        } else if (strcmp(buffer, ENTER_LIBRARY) == 0) {
            token = enter_library(login_cookie);
        } else if (strcmp(buffer, GET_BOOKS) == 0) {
            get_books(token);
        } else if (strcmp(buffer, GET_BOOK) == 0) {
            get_book(token);
        } else if (strcmp(buffer, ADD_BOOK) == 0) {
            add_book(token);
        } else if (strcmp(buffer, DELETE_BOOK) == 0) {
            delete_book(token);
        } else if (strcmp(buffer, LOGOUT) == 0) {
            logout(login_cookie);
            login_cookie = NULL;
            token = NULL;
        } else if (strcmp(buffer, EXIT) == 0) {
            break;
        } else {
            printf("Unknown command!\n");
        }
    }

    return 0;
}
