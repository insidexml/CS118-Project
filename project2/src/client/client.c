#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include "../protocol.h"
#include "../util.h"

#ifndef PC
    #define PC 0.0
#endif

#ifndef PI
    #define PI 0.0
#endif

void send_initial_packet(char *filename, int sockfd, struct sockaddr_in *serv_addr){
    packet* p = packet_init();                //initialize packet
    packet_set_data(p, filename, strlen(filename) + 1);            //set payload to file name
    packet_set_syn(p);                        //set syn value
    packet_send(p, sockfd, serv_addr);        //send packet to server
    packet_free(p);
}

void send_ack(int sockfd, int seq, struct sockaddr_in *serv_addr){
    packet* p = packet_init();
    packet_set_ack(p);
    p->seq = seq;
    packet_send(p, sockfd, serv_addr);
    packet_free(p);
}

void send_fin_ack(int sockfd, struct sockaddr_in *serv_addr){
    packet *p = packet_init();
    packet_set_ack(p);
    packet_set_fin(p);
    packet_send(p, sockfd, serv_addr);
    packet_free(p);
}

int main(int argc, char *argv[])
{
    printf("Client initializing with Pc = %0.2f, Pi = %0.2f\n", PC, PI);

    // Seeding rand
    srand(time(NULL));

    // Checking arguments
    if (argc < 2)
        error("no host provided");
    if (argc < 3)
        error("no port provided");
    if (argc < 4)
        error("no filename provided");

    // Retrieving port
    int portno = atoi(argv[2]);

    // Looking up hostname
    char *hostname = argv[1];
    struct hostent* h = gethostbyname(hostname);
    struct in_addr* serv_ip = (struct in_addr*)h->h_addr;

    // Constructing the socket
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = serv_ip->s_addr;
    serv_addr.sin_port = htons(portno);

    // Opening file to write to
    char *filename = argv[3];
    int filefd = creat(filename, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
    if (filefd < 0)
        error("opening file");

    // Define state machine
    int state = 0;
    int time;
    int expected_seq = 0;
    char buf[PACKET_MAX_SIZE];

    while (1) {

        // Initial request packet
        if (state == 0) {
            tprintf("Sending request packet for \"%s\"\n", filename);
            send_initial_packet(filename, sockfd, &serv_addr);
            time = 100;
            int ready = select_wait(sockfd, &time);
            if (ready)
                state++;
        }

        // Receiving data
        if (state == 1) {
            ssize_t psize = recv(sockfd, buf, PACKET_MAX_SIZE, 0);

            if (rand() < PC * RAND_MAX) {
                tprintf("---------- CORRUPTED datagram ----------\n");
                continue;
            }
            else if (rand() < PI * RAND_MAX) {
                tprintf("---------- LOST datagram ----------\n");
                continue;
            }

            packet *p = packet_extract(buf, psize);
            tprintf("---------- Received datagram ----------\n");
            tprintf("SEQ: %u, length: %d, syn/ack/fin: %d%d%d\n",
                   p->seq, p->len, packet_is_syn(p), packet_is_ack(p),
                   packet_is_fin(p));

            if (packet_is_fin(p)){
                tprintf("End of data\n");
                packet_free(p);
                close(filefd);
                state++;
                tprintf("Sending FIN ACK, initiating timeout...\n");
                send_fin_ack(sockfd, &serv_addr);
                time = 1000;
            }
            else{
                // Correct packet, update state
                if (p->seq == expected_seq) {
                    write(filefd, p->data, p->len);
                    expected_seq += p->len;
                    tprintf("Sending ACK %d\n", expected_seq);
                }
                else {
                    tprintf("WRONG SEQ!\n");
                    tprintf("Resending ACK %d\n", expected_seq);
                }
                send_ack(sockfd, expected_seq, &serv_addr);
                packet_free(p);
            }
        }

        // Received all data
        if (state == 2) {
            int ready = select_wait(sockfd, &time);
            if (!ready)
                break;
            else {
                // Check for fin-ack response
                ssize_t psize = recv(sockfd, buf, PACKET_MAX_SIZE, 0);

                if (rand() < PC * RAND_MAX) {
                    tprintf("---------- CORRUPTED datagram ----------\n");
                    continue;
                }
                else if (rand() < PI * RAND_MAX) {
                    tprintf("---------- LOST datagram ----------\n");
                    continue;
                }

                packet* p = packet_extract(buf, psize);
                tprintf("---------- Received datagram ----------\n");
                tprintf("SEQ: %u, length: %d, syn/ack/fin: %d%d%d\n",
                       p->seq, p->len, packet_is_syn(p), packet_is_ack(p),
                       packet_is_fin(p));

                if (packet_is_ack(p) && packet_is_fin(p)) {
                    packet_free(p);
                    break;
                }
                else if (packet_is_fin(p)) {
                    tprintf("Sending FIN ACK, initiating timeout...\n");
                    send_fin_ack(sockfd, &serv_addr);
                    time = 1000;
                }
                packet_free(p);
            }
        }
    }
    tprintf("Terminating connection\n");
    shutdown(sockfd, SHUT_RDWR);
    return 0;
}
