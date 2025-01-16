#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT (8082)

#define HTTP_200_OK "HTTP/1.1 200 OK\r\n"
#define HTTP_404_ERR "HTTP/1.1 404 PAGE NOT FOUND\r\n"
#define CONTENT_TEXT_HTML "Content-Type: text/html\r\n"

#define SLICE_FMT "%.*s"
#define SLICE_PNT(SLICE) SLICE.len, SLICE.buf
typedef struct Slice
{
    char *buf;
    int len;
} Slice;

typedef struct SlicePair
{
    Slice name, value;
} SlicePair;

#define SLICE(CLIT) \
    (Slice) { .buf = CLIT, .len = sizeof(CLIT) / sizeof(CLIT[0]) }

// non-zero to be not match  ... tbh idk how memcmps output works, also we just match the parts
// of the shortest string, from the from
int slice_cmp(Slice a, Slice b) { return memcmp(a.buf, b.buf, a.len < b.len ? a.len : b.len); }

char *HttpUrlStringSearch(char *buf, int *out_methodlen)
{
    // we read upto the first space
    char *ret = buf;

    for (*out_methodlen = 0;
         buf[*out_methodlen] &&
         buf[*out_methodlen] != ' ' &&
         buf[*out_methodlen] != '=' /* url value seprator */ &&
         buf[*out_methodlen] != '&' /* param seprator */ &&
         buf[*out_methodlen] != '?' /* url param sepprator */;
         *out_methodlen += 1)
        ;

    return ret;
}

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

int Write404(int fd)
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

    return WriteBuffer(fd, error_404_html, sizeof(error_404_html) / sizeof(error_404_html[0]));
#undef error_404_html
}

int WriteHelloWorldHtml(int fd)
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

    return WriteBuffer(fd, hello_world_html, sizeof(hello_world_html) / sizeof(hello_world_html[0]));
#undef hello_world_html
}

int WriteDateTime(int fd)
{
    time_t now;
    time(&now);

    // Convert to local time
    struct tm *local = localtime(&now);

    // Format and print the date and time
    char time_buff[80];
    strftime(time_buff, sizeof(time_buff), "%c", local);

#define datetime_html HTTP_200_OK CONTENT_TEXT_HTML \
    "\r\n<!DOCTYPE html>\r\n"                       \
    "<html>\r\n"                                    \
    "<head></head>\r\n"                             \
    "<body>\r\n"                                    \
    "This was rendered at %s\r\n"                   \
    "</body>\r\n"                                   \
    "</html>\r\n\r\n"

    char outbuf[1024] = {0};

    sprintf(outbuf, datetime_html, time_buff);

    return WriteBuffer(fd, outbuf, strlen(outbuf));
#undef datetime_html
}

int WriteArgsTable(
    int fd,
    int url_argc,
    SlicePair url_args[url_argc])
{

// style copied from https://www.w3schools.com/html/tryit.asp?filename=tryhtml_table_intro
#define args_table_html HTTP_200_OK CONTENT_TEXT_HTML                                                                                                                                                                               \
    "\r\n<!DOCTYPE html>\r\n"                                                                                                                                                                                                       \
    "<html>\r\n"                                                                                                                                                                                                                    \
    "<head><style>table {font-family: arial, sans-serif;border-collapse: collapse;width: 100%;}td, th {border: 1px solid #dddddd;text-align: left;padding: 8px;}tr:nth-child(even) {background-color: #dddddd;}</style></head>\r\n" \
    "<body>\r\n"                                                                                                                                                                                                                    \
    "<table><tr><th>name</th><th>value</th></tr>%s</table>\r\n"                                                                                                                                                                     \
    "</body>\r\n"                                                                                                                                                                                                                   \
    "</html>\r\n\r\n"

    char outbuffer[1024] = {0};
    char builderbuffer[1024] = {0};

#define tr_start "<tr>"
#define tr_end "</tr>"
#define td_start "<td>"
#define td_end "</td>"

    //   <tr>
    //     <td>name</td>
    //     <td>value</td>
    //   </tr>

    int builderidx = 0;

    for (int i = 0; i < url_argc; i++)
    {
        SlicePair p = url_args[i];
        // clang-format off
        for (int i = 0; i < sizeof(tr_start)-1; i++){ builderbuffer[builderidx++] = tr_start[i]; }
        for (int i = 0; i < sizeof(td_start)-1; i++){ builderbuffer[builderidx++] = td_start[i]; }
        for (int i = 0; i < p.name.len; i++){ builderbuffer[builderidx++] = p.name.buf[i]; }
        for (int i = 0; i < sizeof(td_end)-1; i++){ builderbuffer[builderidx++] = td_end[i]; }
        for (int i = 0; i < sizeof(td_start)-1; i++){ builderbuffer[builderidx++] = td_start[i]; }
        for (int i = 0; i < p.value.len; i++){ builderbuffer[builderidx++] = p.value.buf[i]; }
        for (int i = 0; i < sizeof(td_end)-1; i++){ builderbuffer[builderidx++] = td_end[i]; }
        for (int i = 0; i < sizeof(tr_end)-1; i++){ builderbuffer[builderidx++] = tr_end[i]; }
        // clang-format on
    }

    snprintf(outbuffer, sizeof(outbuffer), args_table_html, builderbuffer);

#undef tr_start tr_end td_start td_end

    return WriteBuffer(fd, outbuffer, sizeof(outbuffer));

#undef args_table_html
}

void Route(
    int fd,
    Slice request,
    Slice method,
    Slice route,
    int url_argc,
    SlicePair url_args[url_argc])
{

    // for (int i = 0; i < url_argc; i++)
    // {
    //     SlicePair arg = url_args[i];
    //     printf(SLICE_FMT ":" SLICE_FMT "\n", SLICE_PNT(arg.name), SLICE_PNT(arg.value));
    // }

    if (slice_cmp(SLICE("GET"), method) == 0 && slice_cmp(SLICE("/"), route) == 0)
    {
        WriteHelloWorldHtml(fd);
    }
    else if (slice_cmp(SLICE("GET"), method) == 0 && slice_cmp(SLICE("/now"), route) == 0)
    {
        WriteDateTime(fd);
    }
    else if (slice_cmp(SLICE("GET"), method) == 0 && slice_cmp(SLICE("/args"), route) == 0)
    {
        WriteArgsTable(fd, url_argc, url_args);
    }
    else
    {
        Write404(fd);
    }

    close(fd);
}

/* returns number of args found in buffer and saved to dst
    or -1 on error
*/
int HttpUrlGetArgs(char *buffer, int buflen, SlicePair *dst, int dstlen)
{
    // with args
    // "?something=yes HTTP/1.1"
    // without args
    // " HTTP/1.1"

    int nowlen, dstidx = 0;
    char *now = buffer;
    int kv = 0; /* toggles between 0 and 1 when we are reading keys and values */

    do
    {
        now = HttpUrlStringSearch(now, &nowlen);

        if (*now == ' ')
        {
            /* end of args */
            break;
        }
        else if (*now == '?' || *now == '&')
        {
            now += 1; // skip arg sepprator
        }
        else
        {
            if (kv == 0)
            {
                /* this is a key */
                int namelen;
                char *namestart = HttpUrlStringSearch(now, &namelen);
                dst[dstidx].name = (Slice){.buf = namestart, .len = namelen};
                kv = 1;
                now += namelen + 1; /* + 1 to skip the '='*/
            }
            else if (kv == 1)
            {
                /* this is a key */
                int vallen;
                char *valstart = HttpUrlStringSearch(now, &vallen);
                dst[dstidx].value = (Slice){.buf = valstart, .len = vallen};
                kv = 0;
                now += vallen;
                dstidx += 1; /* we got a kv, time to read another! */

                if (dstidx > dstlen)
                {
                    puts("Client sent more then dstlen args!");
                    return -1;
                }
            }
        }
    } while (*now != ' ');

    // puts(buffer);
    return dstidx;
}

void handle_request(int fd)
{
    char buffer[1024] = {0};
    SlicePair args[128] = {0};

    // i think the first word is always the method, and then the route upto the next space?
    int r = read(fd, buffer, sizeof(buffer));
    if (0 > r)
    {
        perror("read");
        exit(EXIT_FAILURE);
    }

    buffer[r] = '\0';

    // puts(buffer);

    int methodlen;
    int routelen;

    /* thease pointers are slices of buff, and there ends are set by methodlen and route len */
    char *method = HttpUrlStringSearch(buffer, &methodlen);
    char *route = HttpUrlStringSearch(buffer + methodlen + 1, &routelen);
    int used = methodlen + 1 + routelen;

    int argc = HttpUrlGetArgs(buffer + used, r - used, args, sizeof(args) / sizeof(args[0]));

    if (0 > argc)
    {
        close(fd);
        return;
    }

    if (!method || !route)
    {
        close(fd);
        return;
    }

    printf("Method: %.*s\nRoute: %.*s\n", methodlen, method, routelen, route);

    Route(fd,
          (Slice){.buf = buffer, .len = sizeof(buffer)},
          (Slice){.buf = method, .len = methodlen},
          (Slice){.buf = route, .len = routelen},
          argc, args);
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