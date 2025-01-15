#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>

#define PORT (8081)

int main(int argc, char *argv[])
{
    int sfd, clen, cfd;
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
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons(PORT);

    if (0 > bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr)))
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (0 > listen(sfd, 1))
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }


ACCEPT_MORE_CLIENTS:
    puts("Waiting for client to connect");

    if (0 > (cfd = accept(sfd, (struct sockaddr *)&caddr, &clen)))
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    puts("Client Connected");

    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    for (;;)
    {
        int r = read(cfd, buffer, sizeof(buffer));
        if (0 > r)
        {
            perror("read");
            goto DONE;
        }

        int w = 0;

        do
        {
            int o = write(cfd, buffer + w, r - w);
            if (0 > o)
            {
                perror("write");
                goto DONE;
            }
            w += o;
        } while (r > w);

        if (buffer[0] == 'q')
        {
            goto DONE;
        }
    }

DONE:
    puts("Disconnecting client");
    if (0 > close(cfd))
    {
        perror("close");
        exit(EXIT_FAILURE);
    }


    goto ACCEPT_MORE_CLIENTS;
}