#ifndef RESPONSES_H
#define RESPONSES_H

void send_200(int sock, int fd);
void send_400(int sock);
void send_404(int sock);
void send_413(int sock);
void send_505(int sock);

#endif
