#include <stdio.h>
#include <sys/time.h>
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

void gbn_server(char *iface, long port, FILE *fp)
{
    int sock_fd;
    int last = -1;
    char buff[SIZE];
    struct sockaddr_in servr, client_addr;
    header header_server;
    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("socket creation failed\n");
        exit(0);
    }

    int opt = 1;
    setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

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

    bzero(buff, SIZE);
    unsigned len = sizeof(client_addr);
    char buff_intermediate[248];
    //exit Flag
    int flag = 1;
    while (flag)
    {
        if (recvfrom(sock_fd, buff, SIZE, 0, (struct sockaddr *)&client_addr, (unsigned int *)&len))
        {
            bzero(&header_server.packet_no, sizeof(header_server.packet_no));
            memcpy(&header_server, buff, sizeof(header_server));

            int seqno = header_server.packet_no;

            // printf("recieved packet with seq no %d\n", seqno);

            if (seqno == last + 1)
            {
                memcpy(&buff_intermediate, buff + sizeof(header_server), sizeof(buff_intermediate));
                fwrite(buff_intermediate, 1, header_server.data_size, fp);

                // settings for last packet
                if (header_server.data_size < 248)
                {
                    // printf("last packet recieved ... !\n");
                    flag = 0;
                }

                char str[10];
                sprintf(str, "%d", seqno);
                // printf("sending ack--%d\n", seqno);

                sendto(sock_fd, str, sizeof(str), 0, (const struct sockaddr *)&client_addr, len);

                // sending last packet ack 5 times
                if (flag == 0)
                {
                    for (int i = 0; i < 5; i++)
                    {
                        // printf("sending ack--%d\n", seqno);
                        sendto(sock_fd, str, sizeof(str), 0, (const struct sockaddr *)&client_addr, len);
                    }
                }
                last = last + 1;
            }
            else
            {
                char str[10];
                sprintf(str, "%d", last);
                sendto(sock_fd, str, sizeof(str), 0, (const struct sockaddr *)&client_addr, len);
            }
        }
    }
}

void gbn_client(char *host, long port, FILE *fp)
{
    int sock_fd;
    char buff[SIZE];
    struct sockaddr_in servr;

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        printf("socket creation failed");
        exit(0);
    }
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

    int len, flag = 1, temp_size;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1;

    struct timeval timer_start, timer_end;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int base = 0, nextseqno = 0;
    int n = 10;
    int time_out = 4000;
    long time = 0;
    char save_buff[1000][256];
    int last_packet = -1;
    while (flag)
    {

        // printf("window size is %d\n", n);
        // printf("nextseqno--%d,  base - %d\n", nextseqno, base);
        if (nextseqno < base + n && last_packet == -1)
        {
            // temp buffer to read the packet
            char temp_buffer[248];
            bzero(&temp_buffer, 248);

            temp_size = fread(&temp_buffer, 1, 248, fp);

            // printf("block size of %d is read \n", temp_size);
            if (temp_size < 248)
            {
                last_packet = nextseqno;
                // printf("last packet read.....!\n");
            }

            bzero(&buff, SIZE);

            // adding header to send buffer
            header header = {
                .packet_no = nextseqno,
                .data_size = temp_size};
            memcpy(buff, &header, sizeof(header));

            // copying read packet into send buffer
            memcpy(buff + sizeof(header), &temp_buffer, temp_size);

            bzero(&save_buff[nextseqno], 256);

            // clone send buffer to save buffer
            memcpy(save_buff[nextseqno], &buff, sizeof(buff));

            // send (send buffer)
            sendto(sock_fd, &buff, sizeof(buff), 0, (const struct sockaddr *)&servr, sizeof(servr));

            // printf("sent--%d\n", header.packet_no);

            if (nextseqno == base)
            {
                gettimeofday(&timer_start, NULL);
            }
            nextseqno = nextseqno + 1;
        }

        if (recvfrom(sock_fd, (char *)buff, SIZE, MSG_WAITALL, (struct sockaddr *)&servr, (unsigned int *)&len) > 0)
        {
            if (atoi(buff) != -1)
            {
                base = atoi(buff) + 1;
                // printf("Ack---%d\n", base - 1);

                if (last_packet == base - 1)
                {
                    flag = 0;
                }
            }
        }

        gettimeofday(&timer_end, NULL);
        long seconds = (timer_end.tv_sec - timer_start.tv_sec);
        long milli_secs = (timer_end.tv_usec - timer_start.tv_usec);
        time = (1000000 * seconds) + milli_secs;
        // printf("time-%ld\n", time);

        if (time > time_out)
        {
            // timed out

            // decrease window size

            if (n / 2 >= 1)
            {
                n /= 2;
            }

            gettimeofday(&timer_start, NULL);
            for (int i = base; i < nextseqno; i++)
            {
                // printf("resending -%d\n", i);
                sendto(sock_fd, &save_buff[i], sizeof(save_buff[i]), 0, (const struct sockaddr *)&servr, sizeof(servr));
            }
        }
        else
        {
            // increase window size
            n++;
        }
    }
}
