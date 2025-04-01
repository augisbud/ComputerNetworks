#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFLEN 2048

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "USAGE: %s <server_ip> <port>\n", argv[0]);
        exit(1);
    }

    unsigned int port = atoi(argv[2]);
    int s_socket;
    struct sockaddr_in servaddr;
    char recvbuffer[BUFFLEN * 2], sendbuffer[BUFFLEN];

    if (port < 1 || port > 65535)
    {
        printf("ERROR: Invalid port.\n");
        exit(1);
    }

    if ((s_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "ERROR: Cannot create socket.\n");
        exit(1);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    if (inet_aton(argv[1], &servaddr.sin_addr) <= 0)
    {
        fprintf(stderr, "ERROR: Invalid remote IP.\n");
        exit(1);
    }

    if (connect(s_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        fprintf(stderr, "ERROR: Connection failed.\n");
        exit(1);
    }

    printf("Connected to server. Connection will be kept alive for multiple requests.\n");

    while (1)
    {
        printf("Input the string you want to reverse (or type '!exit' to quit): ");
        fgets(sendbuffer, BUFFLEN, stdin);
        sendbuffer[strcspn(sendbuffer, "\n")] = 0;
        
        if (strcmp(sendbuffer, "!exit") == 0)
            break;
        
        if (strlen(sendbuffer) == 0)
        {
            printf("ERROR: Empty input.\n");
            continue;
        }

        char http_request[BUFFLEN * 2];
        snprintf(http_request, sizeof(http_request),
            "POST / HTTP/1.1\r\n"
            "Host: %s:%d\r\n"
            "User-Agent: client-c\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: %ld\r\n"
            "\r\n"
            "%s",
            argv[1], port, strlen(sendbuffer), sendbuffer);
        
        send(s_socket, http_request, strlen(http_request), 0);
        
        memset(recvbuffer, 0, sizeof(recvbuffer));
        int total_received = 0;
        int bytes_received;
        
        while ((bytes_received = recv(s_socket, recvbuffer + total_received, 
                                     sizeof(recvbuffer) - total_received - 1, 0)) > 0)
        {
            total_received += bytes_received;
            if (strstr(recvbuffer, "\r\n\r\n") != NULL)
            {
                char *content_length = strstr(recvbuffer, "Content-Length: ");
                if (content_length)
                {
                    int declared_length = atoi(content_length + 16);
                    char *body_start = strstr(recvbuffer, "\r\n\r\n") + 4;
                    int body_length = total_received - (body_start - recvbuffer);
                    if (body_length >= declared_length)
                    {
                        break;
                    }
                }
                else
                {
                    break;
                }
            }
        }
        
        char *body_start = strstr(recvbuffer, "\r\n\r\n");
        if (body_start != NULL)
        {
            body_start += 4;
            printf("Reversed string: %s\n", body_start);
        }
        else
        {
            printf("Server response:\n%s\n", recvbuffer);
        }
    }

    close(s_socket);
    return 0;
}