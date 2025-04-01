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

void process_http_request(const char *request, char *response)
{
    // Find the request body (after double newline)
    const char *body_start = strstr(request, "\r\n\r\n");
    char input[BUFFLEN] = {0};
    
    if (body_start != NULL)
    {
        body_start += 4; // Skip past the double newline
        strncpy(input, body_start, BUFFLEN - 1);
    }
    
    char reversed[BUFFLEN];
    reverse_string(input, reversed);
    
    // Build HTTP response
    snprintf(response, BUFFLEN * 2,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        strlen(reversed), reversed);
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
                    
                    char http_response[BUFFLEN * 2];
                    process_http_request(buffer, http_response);
                    
                    printf("Sending HTTP response...\n");
                    send(c_sockets[i], http_response, strlen(http_response), 0);
                    
                    // Close connection after sending response (Connection: close)
                    close(c_sockets[i]);
                    c_sockets[i] = -1;
                    printf("User disconnected after response. Users connected: %d\n", count_active_clients(c_sockets));
                }
            }
        }
    }

    return 0;
}