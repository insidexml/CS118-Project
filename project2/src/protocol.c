#include <stdlib.h>
#include <string.h>
#include "protocol.h"

packet* packet_init()
{
    packet* p;

    if ((p = malloc(sizeof(packet))) == 0)
        return NULL;
    memset(p, 0, sizeof(packet));
    p->bufsize = 64;
    if ((p->data = malloc(p->bufsize)) == 0) {
        free(p);
        return NULL;
    }

    return p;
}

packet* packet_extract(char* buf, uint32_t bufsize)
{
    if (bufsize < PACKET_HEADER_SIZE)
        return NULL;

    packet* p = packet_init();
    memcpy(p, buf, PACKET_HEADER_SIZE);

    if (p->len > PACKET_MAX_SIZE - PACKET_HEADER_SIZE ||
        p->len > bufsize - PACKET_HEADER_SIZE) {
        packet_free(p);
        return NULL;
    }

    packet_set_data(p, buf + PACKET_HEADER_SIZE, p->len);

    return p;
}

void packet_free(packet* p)
{
    free(p->data);
    free(p);
}

int packet_send(packet* p, int sockfd, const struct sockaddr_in *dest_addr)
{
    uint8_t buf[PACKET_MAX_SIZE];
    memcpy(buf, p, PACKET_HEADER_SIZE);
    memcpy(buf+PACKET_HEADER_SIZE, p->data, p->len);

    socklen_t addrlen = sizeof(struct sockaddr);
    int err;
    if ((err = sendto(sockfd, buf, PACKET_HEADER_SIZE + p->len, 0, (const struct sockaddr*) dest_addr, addrlen)) != 0)
        return err;

    return 0;
}

int packet_set_syn(packet* p)
{
    p->flags ^= PACKET_SYN;
    return 0;
}

int packet_set_ack(packet* p)
{
    p->flags ^= PACKET_ACK;
    return 0;
}

int packet_set_fin(packet* p)
{
    p->flags ^= PACKET_FIN;
    return 0;
}

int packet_is_syn(packet* p)
{
    return !!(p->flags & PACKET_SYN);
}

int packet_is_ack(packet* p)
{
    return !!(p->flags & PACKET_ACK);
}

int packet_is_fin(packet* p)
{
    return !!(p->flags & PACKET_FIN);
}

int packet_set_data(packet* p, char* buf, uint32_t bufsize)
{
    uint16_t orig = p->len;
    int err;
    p->len = 0;
    if ((err = packet_append_data(p, buf, bufsize)) != 0) {
        p->len = orig;
        return err;
    }

    return 0;
}

int packet_append_data(packet* p, char *buf, uint32_t bufsize)
{
    if (bufsize > PACKET_MAX_SIZE - PACKET_HEADER_SIZE - p->len)
        return -1;

    if (bufsize + p->len > p->bufsize) {
        uint16_t target = p->bufsize;
        while (target < bufsize + p->len)
            target *= 2;
        if ((p->data = realloc(p->data, target)) == 0)
            return -2;
        p->bufsize = target;
    }
    memcpy(p->data + p->len, buf, bufsize);

    p->len += bufsize;

    return 0;
}

