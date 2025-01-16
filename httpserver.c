#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT (8082)

char *HttpUrlStringSearch(char *buf, char search, int *out_methodlen)
{
    // we read upto the first space
    char *ret = buf;

    for (*out_methodlen = 0;
         buf[*out_methodlen] &&
         buf[*out_methodlen] != search &&
         buf[*out_methodlen] != '=' /*url value seprator*/ &&
         buf[*out_methodlen] != '?' /*url param sepprator*/;
         *out_methodlen += 1)
        ;

    return ret;
}

#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_404_ERR "HTTP/1.1 404 PAGE NOT FOUND\r\n"
#define CONTENT_TEXT_HTML "Content-Type: text/html\r\n"

// 0 ok, 0>ret bad
int WriteBuffer(int fd, char *buffer, int bufferlen)
{
    int out = 0;
    do
    {
        int o = write(fd, buffer, bufferlen - out);
        if (0 > o)
        {
            perror("WriteBuffer");
            return o;
        }
        out += o;
    } while (bufferlen > out);
    return 0;
}

void Write404(int fd)
{
#define error_404_html                   \
    HTTP_404_ERR CONTENT_TEXT_HTML       \
        "\r\n<!DOCTYPE html>\r\n"        \
        "<html>\r\n"                     \
        "<head></head>\r\n"              \
        "<body>\r\n"                     \
        "That Page dose not exist!!\r\n" \
        "</body>\r\n"                    \
        "</html>\r\n\r\n"

    WriteBuffer(fd, error_404_html, sizeof(error_404_html) / sizeof(error_404_html[0]));
#undef error_404_html
}

void WriteHelloWorldHtml(int fd)
{
#define hello_world_html          \
    HTTP_200_OK CONTENT_TEXT_HTML \
        "\r\n<!DOCTYPE html>\r\n" \
        "<html>\r\n"              \
        "<head></head>\r\n"       \
        "<body>\r\n"              \
        "something else!\r\n"     \
        "</body>\r\n"             \
        "</html>\r\n\r\n"

    WriteBuffer(fd, hello_world_html, sizeof(hello_world_html) / sizeof(hello_world_html[0]));
#undef hello_world_html
}

void WriteDateTime(int fd)
{
    time_t now;
    time(&now);

    // Convert to local time
    struct tm *local = localtime(&now);

    // Format and print the date and time
    char time_buff[80];
    strftime(time_buff, sizeof(time_buff), "%c", local);

    #define datetime_html HTTP_200_OK CONTENT_TEXT_HTML\
        "\r\n<!DOCTYPE html>\r\n"\
        "<html>\r\n"\
        "<head></head>\r\n"\
        "<body>\r\n"\
        "This was rendered at %s\r\n"\
        "</body>\r\n"\
        "</html>\r\n\r\n"

    char outbuf [1024] = {0};

    sprintf(outbuf, datetime_html, time_buff);

    WriteBuffer(fd, outbuf, strlen(outbuf));

}

#define SLICE_FMT "%.*s"
#define SLICE_PNT(SLICE) SLICE.len, SLICE.base
typedef struct Slice
{
    char *buf;
    int len;
} Slice;
#define SLICE(CLIT) \
    (Slice) { .buf = CLIT, .len = sizeof(CLIT) / sizeof(CLIT[0]) }

#define SLICE_CMP(SLICE, STRINGLITTERAL)

// non-zero to be not match  ... tbh idk how memcmps output works, also we just match the parts
// of the shortest string, from the from
int slice_cmp(Slice a, Slice b) { return memcmp(a.buf, b.buf, a.len < b.len ? a.len : b.len); }

void Route(
    int fd,
    Slice request,
    Slice method,
    Slice route)
{

    if (slice_cmp(SLICE("GET"), method) == 0 && slice_cmp(SLICE("/"), route) == 0)
    {
        WriteHelloWorldHtml(fd);
    }
    else if (slice_cmp(SLICE("GET"), method) == 0 && slice_cmp(SLICE("/now"), route) == 0)
    {
        WriteDateTime(fd);
    }
    else
    {
        Write404(fd);
    }

    close(fd);

    // if (strncmp("GET", method, methodlen) == 0 && strncmp("/", route, routelen) == 0)
    // {
    // }
    // else
    // {
    //     // a 404 page
    //     puts("todo, 404");
    // }
}

void handle_request(int fd)
{
    char buffer[1024] = {0};
    // i think the first word is always the method, and then the route upto the next space?
    int r = read(fd, buffer, sizeof(buffer));
    if (0 > r)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buffer[r] = '\0';

    puts(buffer);

    int methodlen;
    int routelen;

    /* thease pointers are slices of buff, and there ends are set by methodlen and route len */
    char *method = HttpUrlStringSearch(buffer, ' ', &methodlen);
    char *route = HttpUrlStringSearch(buffer + methodlen + 1, ' ', &routelen);
    // char *payload = ;

    if (!method || !route)
    {
        puts("Client sent some invalid http data!");
        exit(EXIT_FAILURE);
    }

    printf("Method %d: %.*s\nRoute %d: %.*s\n", methodlen, methodlen, method, routelen, routelen, route);

    Route(fd,
          (Slice){.buf = buffer, .len = sizeof(buffer)},
          (Slice){.buf = method, .len = methodlen},
          (Slice){.buf = route, .len = routelen});
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