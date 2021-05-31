#ifndef _HELPERS_
#define _HELPERS_

#define BUFLEN 4096
#define LINELEN 1000

#define IPSERVER "34.118.48.238"
#define PORT 8080

#define REGISTER "register\n"
#define LOGIN "login\n"
#define ENTER_LIBRARY "enter_library\n"
#define GET_BOOKS "get_books\n"
#define GET_BOOK "get_book\n"
#define ADD_BOOK "add_book\n"
#define DELETE_BOOK "delete_book\n"
#define LOGOUT "logout\n"
#define EXIT "exit\n"

// shows the current error
void error(const char *msg);

// adds a line to a string message
void compute_message(char *message, const char *line);

// opens a connection with server host_ip on port portno, returns a socket
int open_connection(char *host_ip, int portno, int ip_type, int socket_type,
                    int flag);

// closes a server connection on socket sockfd
void close_connection(int sockfd);

// send a message to a server
void send_to_server(int sockfd, char *message);

// receives and returns the message from a server
char *receive_from_server(int sockfd);

// extracts and returns a JSON from a server response
char *basic_extract_json_response(char *str);

// extracts and returns a cookie from a server response
char *extract_cookie_response(char *str);

// extracts booklist from a server response
char *extract_json_list(char *str);

// extracts error from a server response
char *get_error(char *response);

// extracts JWT token from a server response
char *get_token(char *response);

// executes register and login commands
char *register_login(char *command);

// executes enter_library command
char *enter_library(char *cookie);

// extracts requested books from a server response
void get_books(char *token);

// extracts requested book from a server response
void get_book(char *token);

// adds a book to the library
void add_book(char *token);

// removes a book from the library
void delete_book(char *token);

// executes logout command
void logout(char *cookie);

#endif
