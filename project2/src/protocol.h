#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static const uint32_t PACKET_HEADER_SIZE = 8,
                      PACKET_MAX_SIZE = 1024;

static const uint8_t PACKET_ACK = 0x1,
                     PACKET_FIN = 0x2,
                     PACKET_SYN = 0x4;

typedef struct packet
{
    // Header fields
    uint32_t seq;
    uint16_t len;
    uint16_t flags;

    // Packet payload
    char* data;

    // Bookkeeping variable - won't appear in actual packet
    uint16_t bufsize;
} packet;

// Initialize an empty packet
packet* packet_init();

// Parse a packet out of a buffer of bytes
packet* packet_extract(char* buf, uint32_t bufsize);

// Frees the packet
void packet_free(packet* p);

// Given the socket and destination address, sends out the packet
int packet_send(packet* p, int sockfd, const struct sockaddr_in *dest_addr);

// Toggle the individual flags
int packet_set_syn(packet* p);
int packet_set_ack(packet* p);
int packet_set_fin(packet* p);
int packet_is_syn(packet* p);
int packet_is_ack(packet* p);
int packet_is_fin(packet* p);

// Set the packet payload
// Returns -1 if doing so will cause packet to exceed PACKET_MAX_SIZE
int packet_set_data(packet* p, char* buf, uint32_t bufsize);

// Append more data to the packet payload
// Returns -1 if doing so will cause packet to exceed PACKET_MAX_SIZE
int packet_append_data(packet* p, char* buf, uint32_t bufsize);

#endif
