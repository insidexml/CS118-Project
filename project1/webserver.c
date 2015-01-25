#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include "util.h"
#include "responses.h"

/*
Given a string, find the position of the last character before a CRLF break. If
a CRLF is not found, returns EOF.
*/
size_t readline(const char* buf, size_t size)
{
    const char match[] = {'\r', '\n'};
    int state = 0;
    int i;
    for (i = 0; i < size && state < 2; i++) {
        if (buf[i] == match[state%2])
            state++;
        else
            state = buf[i] == match[0];
    }
    if (state == 2)
        return i-2;
    return EOF;
}

/*
Serves the HTTP request from a single connection. Its job is to parse the
request and then call the appropriate response from those defined in
responses.c
*/
void serve_request(int sock)
{
    /*
    This section validates the request format and extracts each line in the
    HTTP header. It enforces a limit of 32 header lines (including request line
    and empty line), and a size limit of 4096 bytes.
    */
    const size_t MAXHEADER = 32;
    const size_t MAXREQ = 4096;
    size_t header[MAXHEADER];
    char ibuf[MAXREQ+10];
    memset(ibuf, 0, MAXREQ+10);
    size_t ibufpos = 0;
    size_t line = 0;
    size_t linepos = 0;

#ifdef VERBOSE
    printf("--------- Request data: ---------\n");
#endif

    while (1) {

        // Continue reading data from connection
        size_t n = read(sock, ibuf+ibufpos, MAXREQ-ibufpos);

#ifdef VERBOSE
        write(1, ibuf+ibufpos, n);
#endif

        // Socket issue
        if (n < 0)
            error("reading from socket");

        // Client prematurely broke TCP connection
        if (n == 0)
            return;

        // Try to read as many header lines from request as possible
        ibufpos += n;
        char done = 0;
        size_t eol;
        while ((eol = readline(ibuf+linepos, ibufpos-linepos)) != EOF) {
            eol += linepos;
            ibuf[eol] = '\0';
            header[line++] = linepos;
            // End of header
            if (linepos == eol) {
                done = 1;
                break;
            }
            // Too many header lines, return 413
            if (line == MAXHEADER) {
                send_413(sock);
                return;
            }
            linepos = eol+2;
            if (linepos >= ibufpos) break;
        }
        if (done) break;

        // Request too big, return 413
        if (ibufpos == MAXREQ) {
            send_413(sock);
            return;
        }
    }

    /*
    At this point, the request is confirmed to be in mostly valid format, and
    the header lines are all extracted. Try to serve the request.
    */
    char path[MAXREQ+1];
    char version[4];
    memset(path, 0, MAXREQ+1);

    // Set the root to the webroot folder
    strcpy(path, "webroot");

    // Extract requested file path from request line
    if ((sscanf(ibuf+header[0], "GET %s HTTP/%3s", path+7, version)) == 2) {

        // HTTP version must be 1.1, else send 505
        if (strcmp(version, "1.1") != 0) {
            send_505(sock);
            return;
        }

        // Attempt to open file. If not exist, send 404
        size_t plen = strlen(path);
        if (path[plen-1] == '/')
            strcat(path, "index.html");
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            send_404(sock);
            return;
        }

        // Serve file
        send_200(sock, fd);
        close(fd);
    }

    // If no file path can be extracted, the request is badly formatted
    else {
        send_400(sock);
    }
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // If any child process terminates and becomes a zombie, reap it
    reap_children();

    // Constructing the socket
    if (argc < 2)
        error("no port provided");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    // Binding the socket to the port and begin listening
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("binding");
    listen(sockfd, 5);

    // Listen loop
    clilen = sizeof(cli_addr);
    while (1) {

        // New connection
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
            error("accept");

#ifdef VERBOSE
        printf("--------- Received connection from: %s ---------\n", inet_ntoa(cli_addr.sin_addr));
#endif

        // Fork a process to handle the connection
        pid = fork();
        if (pid < 0)
            error("fork");

        // The child closes the irrelevant fds and handles the request
        if (pid == 0)  {
            close(sockfd);
            serve_request(newsockfd);
            close(newsockfd);
            exit(0);
        }
        // The parent closes its copy of the connection and moves on
        else
            close(newsockfd);
    }
    return 0;
}
