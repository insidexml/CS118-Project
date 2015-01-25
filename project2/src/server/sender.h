#ifndef SENDER_H
#define SENDER_H

void serve_request(int sockfd, int unixfd, int cli_ip, int cli_port);

#endif
