
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <netdb.h>
#include "file.h"
#include <sys/time.h>
#include <stdlib.h>
#define SIZE 256
#define F_GETPATH 50

typedef struct
{
    int packet_no;
    int data_size;
} header;

void stopandwait_server(char *iface, long port, FILE *fp)
{
    header header;
    int sock_fd;
    struct sockaddr_in servr, client_addr;
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd == -1)
    {
        printf("socket creation failed");
        exit(0);
    }

    bzero(&servr, sizeof(servr));
    bzero(&client_addr, sizeof(client_addr));

    // assigning IP address and port
    servr.sin_family = AF_INET;
    servr.sin_addr.s_addr = INADDR_ANY;
    servr.sin_port = htons(port);

    if (bind(sock_fd, (const struct sockaddr *)&servr, sizeof(servr)) < 0)
    {
        printf("binding failed\n");
        exit(0);
    }
    char buff[SIZE];
    int temp;
    bzero(buff, SIZE);
    char buff_intermediate[248];
    int received_no = -1;
    unsigned len = sizeof(client_addr);
    // exit flag
    int flag = 1;
    while (flag)
    {
        temp = recvfrom(sock_fd, buff, SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int *)&len);
        memcpy(&header, buff, sizeof(header));
        if (header.data_size < 248)
        {
            flag = 0;
        }
        if (temp <= 0)
        {
            break;
        }
        if (received_no != header.packet_no)
        {
            memcpy(&buff_intermediate, buff + sizeof(header), sizeof(buff_intermediate));
            fwrite(buff_intermediate, 1, header.data_size, fp);
            received_no = header.packet_no;
            char str[10];
            sprintf(str, "%d", header.packet_no);
            sendto(sock_fd, str, sizeof(str), 0, (const struct sockaddr *)&client_addr, len);
            bzero(buff, SIZE);
        }
    }
}

void stopandwait_client(char *host, long port, FILE *fp)
{
    int sock_fd;
    char buff[SIZE];
    char buff_intermediate[248];
    int i = 1;
    struct sockaddr_in servr;
    sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock_fd == -1)
    {
        printf("socket creation failed\n");
        exit(0);
    }
    // code for ip resolution
    struct addrinfo hints, *res;
    int err;

    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;

    if ((err = getaddrinfo(host, "http", &hints, &res)) != 0)
    {
        printf("error %d : %s\n", err, gai_strerror(err));
    }

    memset(&servr, 0, sizeof(servr));
    servr.sin_family = AF_INET;
    servr.sin_port = htons(port);

    struct sockaddr_in *temp;
    temp = (struct sockaddr_in *)res->ai_addr;
    inet_aton(inet_ntoa(temp->sin_addr), &servr.sin_addr);

    int len, temp_size;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int flag = 1;
    while (flag)
    {
        temp_size = fread(buff_intermediate, 1, 248, fp);
        if (temp_size < 248)
        {
            flag = 0;
        }

        header header = {
            .packet_no = i,
            .data_size = temp_size};
        bzero(buff, SIZE);
        memcpy(buff, &header, sizeof(header));
        memcpy(buff + sizeof(header), &buff_intermediate, header.data_size);

        while (1)
        {
            sendto(sock_fd, buff, sizeof(buff), 0, (const struct sockaddr *)&servr, sizeof(servr));
            if (recvfrom(sock_fd, (char *)buff, SIZE, MSG_WAITALL, (struct sockaddr *)&servr, (unsigned int *)&len) > 0)
            {
                if (atoi(buff) == i)
                {
                    // printf("ACK - %d\n", i);
                    break;
                }
            }
        }
        i++;
        i %= 2;
    }

    // Older exit strat
    // strcpy(buff, "END\n");
    // int sending;
    // sending = sendto(sock_fd, buff, temp_size, 0, (const struct sockaddr *)&servr, sizeof(servr));
    // if (sending == -1)
    // {
    //     printf("Error in send.\n");
    //     exit(1);
    // }
    // bzero(buff, SIZE);
}
