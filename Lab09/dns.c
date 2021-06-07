// Protocoale de comunicatii
// Laborator 9 - DNS
// dns.c

#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define SERVLEN 20
#define HOSTLEN 1024

int usage(char* name) {
    printf("Usage:\n\t%s -n <NAME>\n\t%s -a <IP>\n", name, name);
    return 1;
}

// Receives a name and prints IP addresses
void get_ip(char* name) {
    int ret;
    struct addrinfo hints, *result;

    // TODO: set hints
    hints.ai_flags = AI_PASSIVE | AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;

    // TODO: get addresses
    ret = getaddrinfo(name, NULL, &hints, &result);
    if (ret < 0) {
        gai_strerror(ret);
    }

    // TODO: iterate through addresses and print them

    char ip_addr[100];
    while (result != NULL) {
        if (result->ai_family == AF_INET) {
            struct sockaddr_in* addr = (struct sockaddr_in*)result->ai_addr;
            if (inet_ntop(result->ai_family, &addr->sin_addr, ip_addr,
                          sizeof(ip_addr)) != NULL) {
                printf("IP is %s\n", ip_addr);
            }
        } else {
            struct sockaddr_in6* addr = (struct sockaddr_in6*)result->ai_addr;
            if (inet_ntop(result->ai_family, &addr->sin6_addr, ip_addr,
                          sizeof(ip_addr)) != NULL) {
                printf("IP is %s\n", ip_addr);
            }
        }
        result = result->ai_next;
    }

    // TODO: free allocated data
    freeaddrinfo(result);
}

// Receives an address and prints the associated name and service
void get_name(char* ip) {
    int ret;
    struct sockaddr_in addr;
    char host[1024];
    char service[20];

    // TODO: fill in address data
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_aton(ip, &addr.sin_addr);

    // TODO: get name and service
    ret = getnameinfo((struct sockaddr*)&addr, sizeof(struct sockaddr_in), host,
                      HOSTLEN, service, SERVLEN, 0);
    if (ret < 0) {
        gai_strerror(ret);
    }

    // TODO: print name and service
    printf("Name is %s\nService is %s\n", host, service);
}

int main(int argc, char** argv) {
    if (argc < 3) {
        return usage(argv[0]);
    }

    if (strncmp(argv[1], "-n", 2) == 0) {
        get_ip(argv[2]);
    } else if (strncmp(argv[1], "-a", 2) == 0) {
        get_name(argv[2]);
    } else {
        return usage(argv[0]);
    }

    return 0;
}
