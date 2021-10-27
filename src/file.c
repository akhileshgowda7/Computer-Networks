#include "file.h"
#define F_GETPATH 50
/*
 *  Here is the starting point for your netster part.2 definitions. Add the 
 *  appropriate comment header as defined in the code formatting guidelines
 */
/* Add function definitions */
#include "stdio.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#define SIZE 256
#define MAX 800
#define PORT 8080
#define sock struct sockaddr

void sendFile_tcp(FILE *fp, int sock_fd){
    // printf(" S inside");
  char buff[SIZE];
  bzero(buff, SIZE);
  int temp;
  while((temp = fread(buff, 1, SIZE, fp)) > 0) {
    if (send(sock_fd, buff, temp, 0) == -1) {
      printf("Error in send");
      exit(0);
    }
    // printf("%s", buff);
    bzero(buff, SIZE);
  }
}

void recieveFile_tcp(FILE *fp, int sock_fd){
//  printf("R Inside");
  char buff[SIZE];
  int temp;

  while (1) {
      
    temp = recv(sock_fd, buff, SIZE, 0);
    if (temp <= 0){
     printf("Error in recieve");
      break;
    }
    fwrite(buff,1,temp,fp);
    bzero(buff, SIZE);
  }
}

void sendFile_udp(FILE *fp, int sock_fd,struct sockaddr_in servr){
    char buff[SIZE];
    bzero(buff, SIZE);
    unsigned int len = sizeof(servr);
    int temp;
  while((temp = fread(buff, 1, SIZE, fp)) > 0) {
    if (sendto(sock_fd, buff, temp, 0,(struct sockaddr *)&servr, len) == -1) {
      printf("Error in send");
      break;
    }

  }
  strcpy(buff, "END");
    sendto(sock_fd, buff, temp, 0,(struct sockaddr *)&servr, len);
    bzero(buff, SIZE);
}

void recieveFile_udp(FILE *fp,int sock_fd){
    char buff[SIZE];
    struct sockaddr_in client_addr;
    char ip_addr[INET_ADDRSTRLEN];
    int temp;
    inet_ntop(AF_INET, &(client_addr.sin_addr), ip_addr, INET_ADDRSTRLEN);
    while(1){
        socklen_t len = sizeof(client_addr);
        temp=recvfrom(sock_fd, buff, sizeof(buff), 0, (struct sockaddr *)&client_addr, &len);
     if (temp <= 0){
      printf("Error in Receive");
      break;
    }
    if (strcmp(buff, "END") == 0){
     break;
    }
    fwrite(buff,1,temp,fp);
    bzero(buff, SIZE);
}
}

void file_server(char* iface, long port,int use_udp, FILE *fp) {
     if (use_udp == 0)
    {
        int sock_fd, connection_fd;
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
            connection_fd = accept(sock_fd, (sock *)&clnt, &len);
            if (connection_fd < 0)
            {
                printf("server accept failed\n");
                exit(0);
            }
           
        recieveFile_tcp(fp,connection_fd);
        close(sock_fd);
}else
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

      recieveFile_udp(fp,sock_fd);
        close(sock_fd);
    }
}

void file_client(char* host, long port,int use_udp, FILE *fp) {
    // printf("%d",use_udp);
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

        sendFile_tcp(fp,sock_fd);

        // closing the socket
        close(sock_fd);
    }else{

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


        sendFile_udp(fp,sock_fd, servr);

        // closing the socket
        close(sock_fd);

    }
}
