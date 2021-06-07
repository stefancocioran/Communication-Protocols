#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"

#define IPSERVER        "34.118.48.238"
#define PORT            8080
#define WEATHER_HOST    "37.139.20.5"
#define WEATHER_PORT     80


int main(int argc, char *argv[])
{
    char *message;
    char *response;
    int sockfd;
        
    // Ex 1.1: GET dummy from main server
    printf("Ex 1.1: GET dummy from main server\n\n");
    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    message = compute_get_request(IPSERVER, "/api/v1/dummy", NULL, NULL, 0);
    puts(message);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response:\n\n");
    puts(response);
    close_connection(sockfd);

    // Ex 1.2: POST dummy and print response from main server
    printf("\nEx 1.2: POST dummy and print response from main server\n\n");
    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    char *type = "application/x-www-form-urlencoded";
    char *data[2] = {"nume=Cocioran", "prenume=Stefan"};
    
    message = compute_post_request(IPSERVER, "/api/v1/dummy", type, data, 2, NULL, 0);
    puts(message);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response:\n\n");
    puts(response);
    close_connection(sockfd);

    // Ex 2: Login into main server
    printf("\nEx 2: Login into main server\n\n");
    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    char *data_login[2] = {"username=student", "password=student"};
    message = compute_post_request(IPSERVER, "/api/v1/auth/login", type, data_login, 2, NULL, 0);
    puts(message);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response:\n\n");
    puts(response);
    close_connection(sockfd);

    // Ex 3: GET weather key from main server
    printf("Ex 3: GET weather key from main server\n\n");
    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    char *cookie = "connect.sid=s%3APFtRwe2h6rYTpXNyGPvNCSasz-Jc1ZQF.wyyozG5YPeXN25R7ErZm2h13lT1KN%2FPB4UqKYSoeUj8; Path=/; HttpOnly";
    message = compute_get_request(IPSERVER, "/api/v1/weather/key", NULL, &cookie, 1);
    puts(message);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response:\n\n");
    puts(response);
    close_connection(sockfd);

    // Ex 4: GET weather data from OpenWeather API
    printf("\nEx 4: GET weather data from OpenWeather API\n\n");
    sockfd = open_connection(WEATHER_HOST, WEATHER_PORT, AF_INET, SOCK_STREAM, 0);   
    char *params = "lat=44.1086&lon=24.9950&appid=b912dd495585fbf756dc6d8f415a7649";
    
    message = compute_get_request(WEATHER_HOST, "/data/2.5/weather", params, NULL, 0);    
    puts(message);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response:\n\n");
    puts(response);
    close_connection(sockfd);

    // Ex 5: POST weather data for verification to main server
    // Ex 6: Logout from main server
    printf("\nEx 6: Logout from main server\n\n");
    sockfd = open_connection(IPSERVER, PORT, AF_INET, SOCK_STREAM, 0);
    char *key = "b912dd495585fbf756dc6d8f415a7649"; 

    message = compute_get_request(IPSERVER, "/api/v1/auth/logout", key, NULL, 0);
    puts(message);
    send_to_server(sockfd, message);

    response = receive_from_server(sockfd);
    printf("Response:\n\n");
    puts(response);
    close_connection(sockfd);

    // BONUS: make the main server return "Already logged in!"
    // free the allocated data at the end!

    return 0;
}
