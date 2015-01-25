#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include "../protocol.h"
#include "../util.h"
#include "sender.h"

#ifndef PC
    #define PC 0.0
#endif

#ifndef PI
    #define PI 0.0
#endif

void serve_request(int sockfd, int unixfd, int cli_ip, int cli_port)
{
    const int MAX_TIMER = 100;
    const int MAX_PACKETS = 5;

    int state = 0;
    int timer = MAX_TIMER;
    uint32_t wleft = 0, wright = 0;
    int filefd;
    packet* pbuf[MAX_PACKETS];
    int pbufpos = 0, pbufsize = 0;
    char buf[PACKET_MAX_SIZE];

    struct sockaddr_in cli_addr;
    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = cli_ip;
    cli_addr.sin_port = cli_port;

    while (1) {
        int ready = select_wait(unixfd, &timer);

        // ----------- received a packet ----------
        if (ready) {
            // Receive the datagram and extract packet
            ssize_t len = recv(unixfd, buf, PACKET_MAX_SIZE, 0);

            // See if we want to simulate lost/corrupt packet
            if (rand() < PI * RAND_MAX) {
                tprintf("---------- LOST datagram from %s:%d ----------\n",
                       inet_ntoa(cli_addr.sin_addr), ntohs(cli_port));
                continue;
            }
            if (rand() < PC * RAND_MAX) {
               tprintf("---------- CORRUPT datagram from %s:%d ----------\n"
                       , inet_ntoa(cli_addr.sin_addr), ntohs(cli_port));
                continue;
            }

            // If not, announce that server received the datagram
            packet* p = packet_extract(buf, len);
            tprintf("---------- Received datagram from %s:%d ----------\n",
                   inet_ntoa(cli_addr.sin_addr), ntohs(cli_port));
            tprintf("SEQ: %u, length: %d, syn/fin/ack: %d%d%d\n",
                   p->seq, p->len, packet_is_syn(p), packet_is_fin(p),
                   packet_is_ack(p));

            // state 0 - waiting for request
            if (state == 0) {
                if (!packet_is_syn(p) || p->len == 0) {
                    tprintf("Malformatted request message\n");
                    break;
                }
                p->data[p->len-1] = 0;
                tprintf("Request for file \"%s\"\n", (char*)p->data);
                filefd = open((char*)p->data, O_RDONLY);
                if (filefd < 0) {
                    tprintf("File cannot be opened\n");
                    break;
                }
                timer = MAX_TIMER;
                state++;
                tprintf("Begin sending data...\n");
            }

            // state 1 - sending file data
            if (state == 1) {

                // shift window
                if (packet_is_ack(p)) {
                    if (p->seq > wleft) {
                        timer = MAX_TIMER;
                        while (wleft < p->seq) {
                            wleft += pbuf[pbufpos]->len;
                            packet_free(pbuf[pbufpos]);
                            pbufpos = (pbufpos + 1) % MAX_PACKETS;
                            pbufsize--;
                        }
                    }
                }

                // send additional data
                if (filefd >= 0 && pbufsize < MAX_PACKETS) {
                    int pbufend = (pbufpos + pbufsize) % MAX_PACKETS;
                    while (pbufsize < MAX_PACKETS) {
                        len = read(filefd, buf, PACKET_MAX_SIZE - PACKET_HEADER_SIZE);
                        if (len == 0) {
                            tprintf("No more data to read from file\n");
                            close(filefd);
                            filefd = -1;
                            break;
                        }
                        pbuf[pbufend] = packet_init();
                        pbuf[pbufend]->seq = wright;
                        packet_set_data(pbuf[pbufend], buf, len);
                        tprintf("Sending packet SEQ: %u, length: %d\n",
                               pbuf[pbufend]->seq, pbuf[pbufend]->len);
                        packet_send(pbuf[pbufend], sockfd, &cli_addr);
                        wright += pbuf[pbufend]->len;
                        pbufsize++;
                        pbufend = (pbufend + 1) % MAX_PACKETS;
                    }
                }

                // done sending
                if (filefd == -1 && pbufsize == 0) {
                    tprintf("Done sending data\n");
                    state++;
                }
            }

            // state 2 - closing connection
            if (state == 2) {
                if (packet_is_ack(p) && packet_is_fin(p)) {
                    tprintf("Sending FIN ACK packet\n");
                    packet_set_ack(pbuf[0]);
                    packet_send(pbuf[0], sockfd, &cli_addr);
                    packet_free(pbuf[0]);
                    pbufsize--;
                    break;
                }
                else if (pbufsize == 0) {
                    pbuf[0] = packet_init();
                    packet_set_fin(pbuf[0]);
                    packet_send(pbuf[0], sockfd, &cli_addr);
                    pbufsize++;
                    tprintf("Sending FIN packet\n");
                }
            }
        }

        // ---------- timeout ----------
        else {

            timer = MAX_TIMER;

            // state 0 - waiting for request
            if (state == 0) {
                // do nothing
                continue;
            }

            tprintf("---------- Timeout for %s:%d ----------\n",
                   inet_ntoa(cli_addr.sin_addr), ntohs(cli_port));

            // state 1 - sending file data
            if (state == 1) {
                int i, curpos;
                for (i = 0; i < pbufsize; i++) {
                    curpos = (pbufpos+i) % MAX_PACKETS;
                    packet_send(pbuf[curpos], sockfd, &cli_addr);
                    tprintf("Resending packet SEQ: %u, length: %d\n",
                           pbuf[curpos]->seq, pbuf[curpos]->len);
                }
            }

            if (state == 2) {
                tprintf("Resending FIN packet\n");
                packet_send(pbuf[0], sockfd, &cli_addr);
                timer = 50;
            }
        }
    }

    tprintf("Terminating connection\n");
    sleep(1);
}
