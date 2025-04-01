#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>

#define BUFFLEN 2048
#define MAXCLIENTS 10

int findemptyuser(int c_sockets[])
{
    for (int i = 0; i < MAXCLIENTS; i++)
    {
        if (c_sockets[i] == -1)
            return i;
    }
    return -1;
}

int count_active_clients(int c_sockets[])
{
    int count = 0;
    for (int i = 0; i < MAXCLIENTS; i++)
    {
        if (c_sockets[i] != -1)
        {
            count++;
        }
    }
    return count;
}

void reverse_string(const char *string, char *result)
{
    int len = strlen(string);
    for (int i = 0; i < len; i++) {
        result[i] = string[len - i - 1];
    }
    result[len] = '\0';
}

int parse_http_request(const char *request, char *body)
{
    const char *body_start = strstr(request, "\r\n\r\n");
    if (body_start != NULL)
    {
        body_start += 4;
        strncpy(body, body_start, BUFFLEN - 1);
        return 1;
    }
    return 0;
}

void build_http_response(const char *body, char *response)
{
    snprintf(response, BUFFLEN * 2,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %ld\r\n"
        "\r\n"
        "%s",
        strlen(body), body);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "USAGE: %s <port>\n", argv[0]);
        return -1;
    }

    unsigned int port = atoi(argv[1]);
    int l_socket, c_sockets[MAXCLIENTS];
    fd_set read_set;
    struct sockaddr_in servaddr, clientaddr;
    int maxfd = 0;
    char buffer[BUFFLEN * 2];

    if (port < 1 || port > 65535)
    {
        fprintf(stderr, "ERROR: Invalid port.\n");
        return -1;
    }

    if ((l_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "ERROR: Cannot create listening socket.\n");
        return -1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(l_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        fprintf(stderr, "ERROR: Cannot bind socket.\n");
        return -1;
    }

    if (listen(l_socket, 5) < 0)
    {
        fprintf(stderr, "ERROR: Listen failed.\n");
        return -1;
    }

    for (int i = 0; i < MAXCLIENTS; i++)
        c_sockets[i] = -1;

    printf("HTTP reverse string server running on port %d\n", port);

    while (1)
    {
        FD_ZERO(&read_set);
        FD_SET(l_socket, &read_set);
        maxfd = l_socket;

        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (c_sockets[i] != -1)
            {
                FD_SET(c_sockets[i], &read_set);
                if (c_sockets[i] > maxfd)
                    maxfd = c_sockets[i];
            }
        }

        select(maxfd + 1, &read_set, NULL, NULL, NULL);

        if (FD_ISSET(l_socket, &read_set))
        {
            int client_id = findemptyuser(c_sockets);
            if (client_id != -1)
            {
                socklen_t clientaddrlen = sizeof(clientaddr);
                memset(&clientaddr, 0, clientaddrlen);
                c_sockets[client_id] = accept(l_socket, (struct sockaddr *)&clientaddr, &clientaddrlen);
                printf("New user connected. Users connected: %d\n", count_active_clients(c_sockets));
            }
        }

        for (int i = 0; i < MAXCLIENTS; i++)
        {
            if (c_sockets[i] != -1 && FD_ISSET(c_sockets[i], &read_set))
            {
                memset(buffer, 0, sizeof(buffer));
                int r_len = recv(c_sockets[i], buffer, sizeof(buffer) - 1, 0);
                if (r_len <= 0)
                {
                    close(c_sockets[i]);
                    c_sockets[i] = -1;
                    printf("User disconnected. Users connected: %d\n", count_active_clients(c_sockets));
                }
                else
                {
                    buffer[r_len] = '\0';
                    printf("Received HTTP request:\n%s\n", buffer);
                    
                    char body[BUFFLEN];
                    if (parse_http_request(buffer, body))
                    {
                        char reversed[BUFFLEN];
                        reverse_string(body, reversed);
                        
                        char http_response[BUFFLEN * 2];
                        build_http_response(reversed, http_response);
                        
                        printf("Sending HTTP response...\n");
                        send(c_sockets[i], http_response, strlen(http_response), 0);
                    }
                    else
                    {
                        char http_response[BUFFLEN];
                        snprintf(http_response, sizeof(http_response),
                            "HTTP/1.1 400 Bad Request\r\n"
                            "Content-Length: 0\r\n"
                            "\r\n");
                        send(c_sockets[i], http_response, strlen(http_response), 0);
                    }
                }
            }
        }
    }

    return 0;
}