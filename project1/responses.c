#include "responses.h"
#include "util.h"
#include "mime.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
A response object that is basically just a char* buffer, but easier to use
*/
typedef struct response
{
    char* buf;
    size_t bufsize;
    size_t maxbufsize;
} response;

/*
Initializes a response object and returns it
*/
response* r_init()
{
    response* r = malloc(sizeof(response));
    r->bufsize = 0;
    r->maxbufsize = 512;
    r->buf = malloc(r->maxbufsize);
    return r;
}

/*
Frees the response object
*/
void r_free(response* r)
{
    free(r->buf);
}

/*
Sends the response and frees the response object
*/
void r_send(response* r, int sock)
{
    send(sock, r->buf, r->bufsize, 0);

#ifdef VERBOSE
    printf("--------- Response data: --------\n");
    write(1, r->buf, r->bufsize);
    printf("\n");
#endif
}

/*
Write the contents of the buffer into the response, reallocating memory as
needed
*/
void r_write_buf(response* r, char* buf, size_t size)
{
    while (r->bufsize + size + 1 > r->maxbufsize) {
        r->maxbufsize *= 2;
        r->buf = realloc(r->buf, r->maxbufsize);
        if (!r->buf)
            error("Ran out of memory crafting response");
    }
    memcpy(r->buf+r->bufsize, buf, size);
    r->buf[r->bufsize+size] = '\0';
    r->bufsize += size;
}

/*
Write a string to the response
*/
void r_write(response* r, char* msg)
{
    r_write_buf(r, msg, strlen(msg));
}

/*
Writes generic information, i.e. the server name and the current datetime
*/
void r_write_generic_info(response* r)
{
    // Write server
    r_write(r, "Server: Webserver(118)\r\n");

    // Write date
    char buf[50] = "Date: ";
    now(buf+strlen(buf));
    r_write(r, buf);
    r_write(r, "\r\n");
}

/*
Retrieves information about the file descriptor and writes them to the response
header
*/
void r_write_content_info(response* r, int fd)
{
    char buf[100];
    struct stat s;
    fstat(fd, &s);

    // Write last modified time
    r_write(r, "Last-Modified: ");
    to_timestamp(s.st_mtime, buf);
    r_write(r, buf);
    r_write(r, "\r\n");

    // Write content type
    r_write(r, "Content-Type: ");
    deduce_mime_type(fd, buf);
    r_write(r, buf);
    r_write(r, "\r\n");

    // Write content length
    sprintf(buf, "Content-Length: %d\r\n", (int)s.st_size);
    r_write(r, buf);

}

/*
Writes content info about a string as if it's an HTML page into the response
header
*/
void r_write_virtual_page_info(response* r, char* page)
{
    // Write last modified date
    char buf[100] = "Last Modified: ";
    now(buf+strlen(buf));
    r_write(r, buf);
    r_write(r, "\r\n");

    // Write content type
    r_write(r, "Content-Type: text/html\r\n");

    // Write content length
    sprintf(buf, "Content-Length: %d\r\n", strlen(page));
    r_write(r, buf);

}

/*
Given a file descriptor, writes all of its contents into the response buffer
*/
void r_write_file(response* r, int fd)
{
    size_t n;
    char buf[1024];
    while ((n = read(fd, buf, 1024)))
        r_write_buf(r, buf, n);
}

// File successfully served
void send_200(int sock, int fd)
{
    response* r = r_init();

    r_write(r, "HTTP/1.1 200 OK\r\n");
    r_write_generic_info(r);
    r_write_content_info(r, fd);
    r_write(r, "\r\n");
    r_write_file(r, fd);

    r_send(r, sock);
    r_free(r);
}

// Malformed request
void send_400(int sock)
{
    response* r = r_init();

    r_write(r, "HTTP/1.1 400 Bad Request\r\n");
    r_write_generic_info(r);
    r_write(r, "\r\n");

    r_send(r, sock);
    r_free(r);
}

// File not found
// Return a virtual hardcoded 404 page
void send_404(int sock)
{
    char* page = "<!DOCTYPE html>\n"
                 "<html>\n"
                 "<head><title>Error</title></head>\n"
                 "<body><h1>404 Not Found</h1></body>\n"
                 "</html>\n";

    response* r = r_init();

    r_write(r, "HTTP/1.1 404 Not Found\r\n");
    r_write_generic_info(r);
    r_write_virtual_page_info(r, page);
    r_write(r, "\r\n");
    r_write(r, page);

    r_send(r, sock);
    r_free(r);
}

// The request is too big
void send_413(int sock)
{
    response* r = r_init();

    r_write(r, "HTTP/1.1 413 Request Entity Too Large\r\n");
    r_write_generic_info(r);
    r_write(r, "\r\n");

    r_send(r, sock);
    r_free(r);
}

// HTTP version of request must be 1.1
void send_505(int sock)
{
    response* r = r_init();

    r_write(r, "HTTP/1.1 505 HTTP Version Not Supported\r\n");
    r_write_generic_info(r);
    r_write(r, "\r\n");

    r_send(r, sock);
    r_free(r);
}
