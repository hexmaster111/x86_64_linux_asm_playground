#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT (8082)
#define RecyclingBufferLen  (1024)

char *GetABuffer()
{
    static char recycling_buffers[10][RecyclingBufferLen];
    static int current_buffer;

    char *mbuff = recycling_buffers[current_buffer++ % 10];
    memset(mbuff, 0, 1024);
    return mbuff;
}

char *ParseMethod(char *buf, int *out_methodlen)
{
    *out_methodlen = 5;
    return "hello";
}

char *ParseRoute(char *buf, int *out_routelen)
{
    *out_routelen = 5;
    return "world";
}

void handle_request(int fd)
{
    char *buff = GetABuffer();
    int r = read(fd, buff, RecyclingBufferLen);
    if (0 > r)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buff[r] = '\0';

    int methodlen;
    int routelen;

    /* thease pointers are slices of buff, and there ends are set by methodlen and route len */
    char *method = ParseMethod(buff, &methodlen),
         *route = ParseRoute(buff, &routelen);

    if (!method || !route)
    {
        puts("Client sent some invalid http data!");
        exit(EXIT_FAILURE);
    }

    printf("Method: %.*s\nRoute: %.*s\n", methodlen, method, routelen, route);
}

int main(int argc, char *argv[])
{
    int sfd, cfd, addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in saddr, caddr;

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (0 > sfd)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    if (0 > setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

// Idk why intellasence cant find this... i can ctrl click it and it finds it...
// but it can find the other SO_
#ifndef SO_REUSEPORT
#define SO_REUSEPORT (15)
#endif

    if (0 > setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port = htons(PORT);

    if (0 > bind(sfd, (struct sockaddr *)&saddr, addr_len))
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (0 > listen(sfd, 1))
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        cfd = accept(sfd, (struct sockaddr *)&caddr, &addr_len);
        if (0 > cfd)
        {
            perror("accept");
            continue;
        }

        handle_request(cfd);
    }
}