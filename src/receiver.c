#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}


int main(int argc, char **argv) {
    int opt;

    char *stats_filename = NULL;
    char *listen_ip = NULL;
    char *listen_port_err;
    uint16_t listen_port;

    while ((opt = getopt(argc, argv, "s:h")) != -1) {
        switch (opt) {
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    listen_ip = argv[optind];
    listen_port = (uint16_t) strtol(argv[optind + 1], &listen_port_err, 10);
    if (*listen_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    ERROR("Receiver has following arguments: stats_filename is %s, listen_ip is %s, listen_port is %u",
        stats_filename, listen_ip, listen_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    ERROR("This is not an error, %s", "now let's code!");

    struct pollfd mypoll;
    memset(&mypoll, 0, sizeof(mypoll));
    mypoll.fd = 0;
    mypoll.events = POLLIN;
    
    while(1){

        if(poll(&mypoll, 1, 100) == 1){

            struct sockaddr_in6 peer_addr;                      
            memset(&peer_addr, 0, sizeof(peer_addr));           
            peer_addr.sin6_family = AF_INET6;
            peer_addr.sin6_port = htons(listen_port);
            inet_pton(AF_INET6, listen_ip, &peer_addr.sin6_addr);


            int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
            if (sock == -1){
                return -1;
            }

            if(bind(sock, (struct sockaddr *) &peer_addr , sizeof(peer_addr)) == -1){
                return -1;
            }

            socklen_t peer_addr_len = sizeof(peer_addr);
            char buffer[1024];
            ssize_t n_received = recvfrom(sock, buffer, 1024, 0, (struct sockaddr *) &peer_addr, &peer_addr_len);
            if (n_received == -1) {
                return -1;
            }

            close(sock);
            break;
        }

        else{
            continue;
        }
    }

    return EXIT_SUCCESS;
}