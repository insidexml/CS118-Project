#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include "../util.h"
#include "../protocol.h"
#include "sender.h"

#ifndef PC
    #define PC 0.0
#endif

#ifndef PI
    #define PI 0.0
#endif

void check_children(int rtable[][4], int* cur)
{
    int i, status;
    for (i = 0; i < *cur;) {
        if (waitpid(rtable[i][0], &status, WNOHANG) != 0) {
            close(rtable[i][3]);
            memcpy(rtable[i], rtable[*cur-1], sizeof(rtable[i]));
            (*cur)--;
        }
        else i++;
    }
}

int main(int argc, char *argv[])
{
    printf("Server initializing with Pc = %0.2f, Pi = %0.2f\n", PC, PI);

    // Seeding rand 
    srand(time(0));

    // Read input
    if (argc < 2)
        error("no port provided");
    int portno = atoi(argv[1]);


    // Constructing the socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("opening socket");

    // Binding the socket to the port
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("binding");

    // Constructing the multiplexer
    const int MAX_REQUESTS = 8;
    int cur_requests = 0;
    int request_table[MAX_REQUESTS][4];

    // Listen loop
    struct sockaddr_in cli_addr;
    socklen_t clilen = sizeof(cli_addr);
    char buf[PACKET_MAX_SIZE];
    int i;
    while (1) {

        // Receive datagram
        ssize_t psize = recvfrom(sockfd, buf, PACKET_MAX_SIZE, 0, (struct sockaddr *) &cli_addr, &clilen);
        int cli_ip = cli_addr.sin_addr.s_addr;
        int cli_port = cli_addr.sin_port;

        // Deregister all zombie children
        check_children(request_table, &cur_requests);

        // Switch the packet to the right child process
        char found = 0;
        for (i = 0; !found && i < cur_requests; i++) {
            found = cli_ip == request_table[i][1] &&
                    cli_port == request_table[i][2];
            if (found) {
                found = send(request_table[i][3], buf, psize, 0) >= 0;
            }
        }

        // Fork new process to handle new client
        if (!found && cur_requests != MAX_REQUESTS) {

            // Set up unix datagram socket
            int fd[2];
            if (socketpair(AF_UNIX, SOCK_DGRAM, 0, fd) < 0)
                error("socketpair");

            // Fork
            int pid = fork();
            if (pid < 0)
                error("fork");

            // The child closes the irrelevant fds and handles the request
            if (pid == 0)  {
                close(fd[0]);
                serve_request(sockfd, fd[1], cli_ip, cli_port);
                close(sockfd);
                close(fd[1]);
                exit(0);
            }

            // The parent registers the child in the request table
            else {
                close(fd[1]);
                request_table[cur_requests][0] = pid;
                request_table[cur_requests][1] = cli_ip;
                request_table[cur_requests][2] = cli_port;
                request_table[cur_requests][3] = fd[0];
                cur_requests++;
                send(fd[0], buf, psize, 0);
            }
        }
    }
    return 0;
}
