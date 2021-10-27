/* Add function definitions */
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#define MSG_CONFIRM 0
#define MAX 800
#define PORT 8080
#define sock struct sockaddr

const int RUN_STATE = 0;
const int CLOSE_STATE = 1;
const int EXIT_STATE = 2;

struct parameters
{
    int connection_fd;
    int n;
    struct sockaddr_in clnt;
};

void *tcp_server(void *arg)
{
    char buffer[MAX];
    struct parameters param = *((struct parameters *)arg);
    int connection_fd = param.connection_fd;
    int n = param.n;
    struct sockaddr_in clnt = param.clnt;
    int curr_state = RUN_STATE;
    char ip_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clnt.sin_addr), ip_addr, INET_ADDRSTRLEN);

    printf("connection %d from ('%s', %d)\n", n, ip_addr, clnt.sin_port);
    // starting an infinite loop for the chat
    for (;;)
    {
        bzero(buffer, MAX);
        int read_buffer, write_buffer = 0;
        // reading messages from client and copyingg it to the server
        read_buffer = recv(connection_fd, buffer, sizeof(buffer), 0);
        printf("got message from ('%s', %d)\n", ip_addr, clnt.sin_port);
        //printf("%d", read_buffer);
        // outputing the contents of the buffer
        // printf("From client: %s", buffer);
        if (read_buffer < 0)
        {
            printf("read error");
            curr_state = CLOSE_STATE;
            break;
        }
        else if (read_buffer == 0)
        {
            // printf("client closed");
            curr_state = CLOSE_STATE;
            break;
        }
        if (strcmp(buffer, "goodbye\n") == 0)
        {
            strcpy(buffer, "farewell\n");
            write_buffer = send(connection_fd, buffer, sizeof(buffer), 0);
            curr_state = CLOSE_STATE;
        }
        else if (strcmp(buffer, "hello\n") == 0)
        {
            strcpy(buffer, "world\n");
            write_buffer = send(connection_fd, buffer, sizeof(buffer), 0);
        }
        else if (strcmp("exit\n", buffer) == 0)
        {
            strcpy(buffer, "ok\n");
            write_buffer = send(connection_fd, buffer, sizeof(buffer), 0);
            curr_state = EXIT_STATE;
            exit(0);
        }
        else
        {
            write_buffer = send(connection_fd, buffer, sizeof(buffer), 0);
        }

        if (write_buffer < 0)
        {
            printf("write error");
        }
        // printf("To client: %s", buffer);

        if (curr_state == CLOSE_STATE)
        {
            pthread_exit(NULL);
        }
        else if (curr_state == EXIT_STATE)
        {
            exit(0);
        }
    }
    return NULL;
}

int udp_server(int sock_fd)
{
    char buffer[MAX];
    struct sockaddr_in client_addr;
    int curr_state = RUN_STATE;
    char ip_addr[INET_ADDRSTRLEN];

    // starting an infinite loop for the chat
    for (;;)
    {
        bzero(buffer, MAX);
        int read_buffer, write_buffer = 0;
        socklen_t len = sizeof(client_addr);
        // reading messages from client and copyingg it to the server
        read_buffer = recvfrom(sock_fd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&client_addr, &len);
        inet_ntop(AF_INET, &(client_addr.sin_addr), ip_addr, INET_ADDRSTRLEN);
        printf("got message from ('%s', %d)\n", ip_addr, client_addr.sin_port);
        if (read_buffer < 0)
        {
            printf("read error\n");
            curr_state = CLOSE_STATE;
            break;
        }
        else if (read_buffer == 0)
        {
            curr_state = CLOSE_STATE;
            break;
        }
        printf("got message from ('%s', %d)\n", ip_addr, client_addr.sin_port);
        if (strcmp(buffer, "goodbye\n") == 0)
        {
            strcpy(buffer, "farewell\n");
            write_buffer = sendto(sock_fd, buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr *)&client_addr, len);
        }
        else if (strcmp(buffer, "hello\n") == 0)
        {
            strcpy(buffer, "world\n");
            write_buffer = sendto(sock_fd, buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr *)&client_addr, len);
        }
        else if (strcmp("exit\n", buffer) == 0)
        {
            strcpy(buffer, "ok\n");
            write_buffer = sendto(sock_fd, buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr *)&client_addr, len);
            curr_state = EXIT_STATE;
            exit(0);
        }
        else
        {
            write_buffer = sendto(sock_fd, buffer, sizeof(buffer), MSG_CONFIRM, (struct sockaddr *)&client_addr, len);
        }

        if (write_buffer < 0)
        {
            printf("write error\n");
        }
        // printf("To client: %s", buffer);
        if (curr_state != RUN_STATE)
        {
            break;
        }
    }
    return curr_state;
}
void tcp_client(int sock_fd)
{

    char buffer[MAX];
    char input_buffer[MAX];

    int read_buffer, write_buffer;
    for (;;)
    {
        bzero(buffer, sizeof(buffer));
        bzero(input_buffer, sizeof(input_buffer));
        //printf("\nEnter the Message : ");
        int i = 0;
        while ((input_buffer[i++] = getchar()) != '\n')
            ;

        write_buffer = send(sock_fd, input_buffer, sizeof(input_buffer), 0);
        if (write_buffer < 0)
        {
            printf("write error client");
        }
        read_buffer = recv(sock_fd, buffer, sizeof(buffer), 0);
        if (read_buffer < 0)
        {
            printf("read error client");
        }
        printf("%s", buffer);
        if (strcmp(input_buffer, "exit\n") == 0 || strcmp(input_buffer, "goodbye\n") == 0)
        {
            // printf("Client Exit\n");
            break;
        }
    }
}
void udp_client(int sock_fd, struct sockaddr_in servr)
{
    char buffer[MAX];
    char input_buffer[MAX];

    int read_buffer, write_buffer;
    for (;;)
    {

        unsigned int len = sizeof(servr);
        bzero(buffer, sizeof(buffer));
        bzero(input_buffer, sizeof(input_buffer));
        int i = 0;
        while ((input_buffer[i++] = getchar()) != '\n')
            ;

        write_buffer = sendto(sock_fd, input_buffer, sizeof(input_buffer), 0, (struct sockaddr *)&servr, len);
        if (write_buffer < 0)
        {
            printf("write error client");
        }
        read_buffer = recvfrom(sock_fd, buffer, sizeof(buffer), MSG_WAITALL, (struct sockaddr *)&servr, &len);
        if (read_buffer < 0)
        {
            printf("read error client");
        }
        printf("%s", buffer);
        if (strcmp(input_buffer, "exit\n") == 0 || strcmp(input_buffer, "goodbye\n") == 0)
        {
            // printf("Client Exit\n");
            break;
        }
    }
}
void chat_server(char *iface, long port, int use_udp)
{
    if (use_udp == 0)
    {
        int sock_fd, connection_fd;
        int n = 0;

        struct sockaddr_in servr, clnt;

        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1)
        {
            printf("socket creation failed\n");
            exit(0);
        }
        else
        {
            // printf("Socket created successfully\n");
        }
        bzero(&servr, sizeof(servr));

        servr.sin_family = AF_INET;
        servr.sin_addr.s_addr = htonl(INADDR_ANY);
        servr.sin_port = htons(port);

        if ((bind(sock_fd, (sock *)&servr, sizeof(servr))) != 0)
        {
            printf("socket binding failed\n");
            exit(0);
        }
        else
        {
            // printf("Socket binded successfully\n");
        }

        if ((listen(sock_fd, 5)) != 0)
        {
            printf("Listen failed\n");
            exit(0);
        }
        else
        {
            // printf("Server listening\n");
        }

        unsigned int len = sizeof(clnt);

        pthread_t threads[10];

        for (;;)
        {
            connection_fd = accept(sock_fd, (sock *)&clnt, &len);
            if (connection_fd < 0)
            {
                printf("server accept failed\n");
                exit(0);
            }
            else
            {
                //printf("server accepted the client\n");
            }

            struct parameters param;
            param.connection_fd = connection_fd;
            param.n = n;
            param.clnt = clnt;

            if (pthread_create(&threads[n], NULL, tcp_server, &param) != 0)
            {
                printf("Thread was not created\n");
            }

            n += 1;
        }
        close(sock_fd);
    }
    else
    {

        int sock_fd;

        struct sockaddr_in servr;

        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd == -1)
        {
            printf("socket creation failed\n");
            exit(0);
        }
        bzero(&servr, sizeof(servr));

        servr.sin_family = AF_INET;
        servr.sin_addr.s_addr = htonl(INADDR_ANY);
        servr.sin_port = htons(port);

        if ((bind(sock_fd, (sock *)&servr, sizeof(servr))) != 0)
        {
            printf("socket binding failed\n");
            exit(0);
        }

        udp_server(sock_fd);

        close(sock_fd);
    }
}

void chat_client(char *host, long port, int use_udp)
{
    if (use_udp == 0)
    {
        int sock_fd;
        struct sockaddr_in servr;

        // creating socket
        sock_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (sock_fd == -1)
        {
            printf("socket creation failed\n");
            exit(0);
        }
        else
            //printf("Socket created successfully\n");
            bzero(&servr, sizeof(servr));

        // assigning IP address and port
        servr.sin_family = AF_INET;
        servr.sin_addr.s_addr = inet_addr("127.0.0.1");
        servr.sin_port = htons(port);

        // connecting the client socket to the server socket
        if (connect(sock_fd, (sock *)&servr, sizeof(servr)) != 0)
        {
            printf("connection with the server failed\n");
            exit(0);
        }
        // else
        // printf("connected to the server\n");

        // calling the client chat function
        tcp_client(sock_fd);

        // closing the socket
        close(sock_fd);
    }
    else
    {
        int sock_fd;
        struct sockaddr_in servr;
        // creating socket
        sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_fd == -1)
        {
            printf("socket creation failed\n");
            exit(0);
        }
        bzero(&servr, sizeof(servr));

        // assigning IP address and port
        servr.sin_family = AF_INET;
        servr.sin_addr.s_addr = inet_addr("127.0.0.1");

        servr.sin_port = htons(port);

        // connecting the client socket to the server socket

        // calling the client chat function
        udp_client(sock_fd, servr);

        // closing the socket
        close(sock_fd);
    }
}
