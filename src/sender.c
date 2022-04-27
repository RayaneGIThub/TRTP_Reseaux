#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "log.h"

int print_usage(char *prog_name) {
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] [-c] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}



void selective_repeat_sender(char* filename, bool fec, int sock){
    
}


int main(int argc, char **argv) {
    int opt;

    char *filename = NULL;
    char *stats_filename = NULL;
    char *receiver_ip = NULL;
    char *receiver_port_err;
    bool fec_enabled = false;
    uint16_t receiver_port;

    while ((opt = getopt(argc, argv, "f:s:hc")) != -1) {
        switch (opt) {
        case 'f':
            filename = optarg;
            break;
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
    case 'c':
        fec_enabled = true;
        break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    receiver_ip = argv[optind];
    receiver_port = (uint16_t) strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    ASSERT(1 == 1); // Try to change it to see what happens when it fails
    DEBUG_DUMP("Some bytes", 11); // You can use it with any pointer type

    // This is not an error per-se.
    ERROR("Sender has following arguments: filename is %s, stats_filename is %s, fec_enabled is %d, receiver_ip is %s, receiver_port is %u",
        filename, stats_filename, fec_enabled, receiver_ip, receiver_port);

    DEBUG("You can only see me if %s", "you built me using `make debug`");
    ERROR("This is not an error, %s", "now let's code!");

    // Now let's code!

    int socket_desc;
    struct sockaddr_in6 server_addr;
    int server_struct_length = sizeof(server_addr);
    
    
    // Create socket:
    socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    
    if(socket_desc < 0){
        printf("Error while creating socket\n");
        return -1;
    }
    printf("Socket created successfully\n");
    
    // Set port and IP:

    memset(&server_addr, 0, sizeof(server_addr));           
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(receiver_port);
    inet_pton(AF_INET6, receiver_ip, &server_addr.sin6_addr);
    
    // Send the message to server:
    if(sendto(socket_desc, (const char *)filename, strlen(filename), 0,(const struct sockaddr*)&server_addr, server_struct_length) < 0){
        printf("Unable to send message\n");
        return -1;
    };
    
    // Close the socket:
    close(socket_desc);
}