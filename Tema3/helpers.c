#include "helpers.h"

#include <arpa/inet.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <stdio.h>
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

#include "buffer.h"
#include "parson.h"
#include "requests.h"

#define HEADER_TERMINATOR "\r\n\r\n"
#define HEADER_TERMINATOR_SIZE (sizeof(HEADER_TERMINATOR) - 1)
#define CONTENT_LENGTH "Content-Length: "
#define CONTENT_LENGTH_SIZE (sizeof(CONTENT_LENGTH) - 1)

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void compute_message(char *message, const char *line) {
    strcat(message, line);
    strcat(message, "\r\n");
}

int open_connection(char *host_ip, int portno, int ip_type, int socket_type,
                    int flag) {
    struct sockaddr_in serv_addr;
    int sockfd = socket(ip_type, socket_type, flag);
    if (sockfd < 0) error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = ip_type;
    serv_addr.sin_port = htons(portno);
    inet_aton(host_ip, &serv_addr.sin_addr);

    /* connect the socket */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    return sockfd;
}

void close_connection(int sockfd) { close(sockfd); }

void send_to_server(int sockfd, char *message) {
    int bytes, sent = 0;
    int total = strlen(message);

    do {
        bytes = write(sockfd, message + sent, total - sent);
        if (bytes < 0) {
            error("ERROR writing message to socket");
        }

        if (bytes == 0) {
            break;
        }

        sent += bytes;
    } while (sent < total);
}

char *receive_from_server(int sockfd) {
    char response[BUFLEN];
    buffer buffer = buffer_init();
    int header_end = 0;
    int content_length = 0;

    do {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t)bytes);

        header_end =
            buffer_find(&buffer, HEADER_TERMINATOR, HEADER_TERMINATOR_SIZE);

        if (header_end >= 0) {
            header_end += HEADER_TERMINATOR_SIZE;

            int content_length_start = buffer_find_insensitive(
                &buffer, CONTENT_LENGTH, CONTENT_LENGTH_SIZE);

            if (content_length_start < 0) {
                continue;
            }

            content_length_start += CONTENT_LENGTH_SIZE;
            content_length =
                strtol(buffer.data + content_length_start, NULL, 10);
            break;
        }
    } while (1);
    size_t total = content_length + (size_t)header_end;

    while (buffer.size < total) {
        int bytes = read(sockfd, response, BUFLEN);

        if (bytes < 0) {
            error("ERROR reading response from socket");
        }

        if (bytes == 0) {
            break;
        }

        buffer_add(&buffer, response, (size_t)bytes);
    }
    buffer_add(&buffer, "", 1);
    return buffer.data;
}

char *basic_extract_json_response(char *str) { return strstr(str, "{\""); }

char *extract_json_list(char *str) { return strstr(str, "["); }

char *extract_cookie_response(char *str) {
    const char *p1 = "Set-Cookie: ";
    const char *p2 = "\r\nDate:";
    const char *i1 = strstr(str, p1);
    if (i1 != NULL) {
        const size_t pl1 = strlen(p1);
        const char *i2 = strstr(i1 + pl1, p2);
        if (p2 != NULL) {
            const size_t mlen = i2 - (i1 + pl1);
            char *ret = malloc(mlen + 1);
            if (ret != NULL) {
                memcpy(ret, i1 + pl1, mlen);
                ret[mlen] = '\0';
                return ret;
            }
        }
    }
    return NULL;
}

char *get_error(char *response) {
    const char s[2] = "\":";
    char *token;
    token = strtok(response, s);
    token = strtok(NULL, s);
    if (strcmp(token, "error") == 0) {
        token = strtok(NULL, s);
        return token;
    }
    return NULL;
}

char *get_token(char *response) {
    const char s[2] = "\":";
    char *token;
    token = strtok(response, s);
    return token;
}

char *register_login(char *command) {
    int sockfd;
    char username[BUFLEN];
    char password[BUFLEN];
    char *response;
    char *message;
    char *payload = NULL;
    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);

    printf("username=");
    fgets(username, BUFLEN - 1, stdin);
    username[strlen(username) - 1] = 0;
    printf("password=");
    fgets(password, BUFLEN - 1, stdin);
    password[strlen(password) - 1] = 0;

    json_object_set_string(obj, "username", username);
    json_object_set_string(obj, "password", password);
    payload = json_serialize_to_string_pretty(val);

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    char *type = "application/json";
    char *data[1] = {payload};

    message =
        compute_post_request(IPSERVER, command, type, data, 1, NULL, 0, NULL);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // if the server response contains the '{' character it means that an error
    // was encountered
    if (strchr(response, '{') != NULL) {
        char *res = basic_extract_json_response(response);
        char *error = get_error(res);
        if (error != NULL) {
            printf("Error! %s\n", error);
            return NULL;
        }
    } else {
        // check if the given command was "login" and extract the cookie
        if (strcmp(command, "/api/v1/tema/auth/login") == 0) {
            printf("Logged in successfully!\n");
            return extract_cookie_response(response);
        } else {
            printf("Registered successfully!\n");
        }
    }

    return NULL;
}

char *enter_library(char *cookie) {
    int sockfd;
    char *message;
    char *response;

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(IPSERVER, "/api/v1/tema/library/access", NULL,
                                  &cookie, 1, NULL);

    send_to_server(sockfd, message);
    response = receive_from_server(sockfd);

    close_connection(sockfd);

    char *res = basic_extract_json_response(response);
    // check if the server responded with an error
    char *error = get_error(res);
    if (error != NULL) {
        printf("Error! %s\n", error);
        return NULL;
    }
    // return JWT Token
    printf("Successfully entered the library!\n");
    return get_token(basic_extract_json_response(response));
}

void get_books(char *token) {
    int sockfd;
    char *message;
    char *response;

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request(IPSERVER, "/api/v1/tema/library/books", NULL,
                                  NULL, 0, token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // if the server response contains the '[' character it means that the
    // server responded with a list of JSON objects (books)
    if (strchr(response, '[') != NULL) {
        char *res = extract_json_list(response);
        printf("%s\n", res);
    } else {
        // otherwise, the server responded with an error
        char *res = basic_extract_json_response(response);
        char *error = get_error(res);
        if (error != NULL) {
            printf("Error! %s\n", error);
        }
    }
}

void get_book(char *token) {
    int sockfd;
    char *response;
    char *message;
    char id[BUFLEN];

    printf("id=");
    fgets(id, BUFLEN - 1, stdin);
    id[strlen(id) - 1] = '\0';

    char *url = calloc(BUFLEN, sizeof(char));
    sprintf(url, "/api/v1/tema/library/books/");
    strcat(url, id);

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request(IPSERVER, url, NULL, NULL, 0, token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // if the server response contains the '[' character it means that the
    // server responded with a list of JSON objects that cointain only one
    // element (the requested book)
    if (strchr(response, '[') != NULL) {
        char *res = basic_extract_json_response(response);
        res[strlen(res) - 1] = '\0';
        printf("%s\n", res);
    } else {
        char *res = basic_extract_json_response(response);
        char *error = get_error(res);
        if (error != NULL) {
            printf("Error! %s\n", error);
        }
    }
}

void add_book(char *token) {
    int sockfd;
    char title[BUFLEN];
    char author[BUFLEN];
    char genre[BUFLEN];
    char publisher[BUFLEN];
    char page_count[BUFLEN];
    char *response;
    char *message;
    char *payload = NULL;
    JSON_Value *val = json_value_init_object();
    JSON_Object *obj = json_value_get_object(val);

    printf("title=");
    fgets(title, BUFLEN - 1, stdin);
    title[strlen(title) - 1] = 0;

    printf("author=");
    fgets(author, BUFLEN - 1, stdin);
    author[strlen(author) - 1] = 0;

    printf("genre=");
    fgets(genre, BUFLEN - 1, stdin);
    genre[strlen(genre) - 1] = 0;

    printf("publisher=");
    fgets(publisher, BUFLEN - 1, stdin);
    publisher[strlen(publisher) - 1] = 0;

    printf("page_count=");
    fgets(page_count, BUFLEN - 1, stdin);
    page_count[strlen(page_count) - 1] = 0;

    json_object_set_string(obj, "title", title);
    json_object_set_string(obj, "author", author);
    json_object_set_string(obj, "genre", genre);
    json_object_set_string(obj, "publisher", publisher);
    json_object_set_string(obj, "page_count", page_count);

    payload = json_serialize_to_string_pretty(val);

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    char *type = "application/json";
    char *data[1] = {payload};

    message = compute_post_request(IPSERVER, "/api/v1/tema/library/books", type,
                                   data, 1, NULL, 0, token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // if the server response contains the '{' character it means that an error
    // was encountered
    if (strchr(response, '{') != NULL) {
        char *res = basic_extract_json_response(response);
        char *error = get_error(res);
        if (error != NULL) {
            printf("Error! %s\n", error);
        }
    } else {
        printf("The book was added!\n");
    }
}

void delete_book(char *token) {
    int sockfd;
    char *response;
    char *message;
    char id[BUFLEN];

    printf("id=");
    fgets(id, BUFLEN - 1, stdin);
    id[strlen(id) - 1] = '\0';

    char *url = calloc(BUFLEN, sizeof(char));
    sprintf(url, "/api/v1/tema/library/books/");
    strcat(url, id);

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);

    message = compute_delete_request(IPSERVER, url, token);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    close_connection(sockfd);

    // if the server response contains the '{' character it means that an error
    // was encountered
    if (strchr(response, '{') != NULL) {
        char *res = basic_extract_json_response(response);
        char *error = get_error(res);
        if (error != NULL) {
            printf("Error! %s\n", error);
        }
    } else {
        printf("The book was removed!\n");
    }
}

void logout(char *cookie) {
    int sockfd;
    char *message;
    char *response;

    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);

    message = compute_get_request(IPSERVER, "/api/v1/tema/auth/logout", NULL,
                                  &cookie, 1, NULL);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);

    // if the server response contains the '{' character it means that an error
    // was encountered
    if (strchr(response, '{') != NULL) {
        char *res = basic_extract_json_response(response);
        char *error = get_error(res);
        if (error != NULL) {
            printf("Error! %s\n", error);
        }
    } else {
        printf("You have been successfully logged out!\n");
    }

    close_connection(sockfd);
}
