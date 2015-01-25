#include <unistd.h>
#include <stdio.h>
#include <string.h>

/*
Given a filename, retrieve its extension (everything after the last '.')
*/
char* get_extension(char* str)
{
    char* file = strrchr(str, '/');
    if (!file)
        file = str;
    else
        file += 1;
    char* ext = strrchr(file, '.');
    if (!ext)
        return 0;
    return ext+1;
}

/*
Given a file descriptor, figure out what MIME type correlates best with the
file. In essence, all it does is look at the filename extension and map
that extension to a known MIME type.
*/
void deduce_mime_type(int fd, char* buf)
{
    // Witchcraft to extract filename from file descriptor
    char filename[101];
    char procpath[50];
    sprintf(procpath, "/proc/self/fd/%d", fd);
    size_t n = readlink(procpath, filename, 100);
    filename[n] = 0;

    // Retrieve the extension of the filename
    char* ext = get_extension(filename);

    // If not extension, default to binary
    if (!ext)
        ext = "bin";

    // Extension to MIME mapping
    char* MIME[][2] = {
        {"html", "text/html"},
        {"bin", "application/octet-stream"},
        {"gif", "image/gif"},
        {"png", "image/png"},
        {"jpg", "image/jpeg"},
        {"jpeg", "image/jpeg"},
        {"txt", "text/plain"},
        {"js", "application/javascript"},
        {"css", "text/css"},
        {"pdf", "application/pdf"},
        {"xml", "application/xml"},
        {"mp3", "audio/mpeg"},
        {"ogg", "audio/ogg"},
        {"json", "application/json"},
        {"zip", "application/zip"},
        {"avi", "video/avi"},
        {"mp4", "video/mp4"}
    };
    int types = sizeof(MIME) / sizeof(MIME[0]);
    int i;
    for (i = 0; i < types; i++) {
        if (strcmp(ext, MIME[i][0]) == 0) {
            strcpy(buf, MIME[i][1]);
            break;
        }
    }
    if (i == types)
        strcpy(buf, "application/octet-stream");
}
