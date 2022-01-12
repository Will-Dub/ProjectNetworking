// This file is make out of funtion for a computer running linux
// HELP

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

void *initS(char ip[], int port, char b){
    struct sockaddr_in server_address;
    //Create sock
    int *sock = (int *)malloc(sizeof(int));
    *sock=1;
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        return NULL;
    }
    // Specify the ip type
    server_address.sin_family = AF_INET;
    // Specifie the port and format the ip so it's allowed later on
    server_address.sin_port = htons(port);
    if(inet_pton(AF_INET, ip, &server_address.sin_addr.s_addr) <= 0) 
    {
        puts("Error in idfk what");
        return NULL;
    }

    // Handle if it's a server or client(use char cuz 1 byte)
    if(b==1){
        if (connect(*sock, (struct sockaddr *)&server_address, sizeof(server_address)) < 0){
            puts("E. Connecting to server");
            return NULL;
        }
    }
    else{
        int opt = 1;
        // Optimize socket
        if(setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) != 0){
            puts("E. Optimizing socket");
            return NULL;
        }
        // Bind the server
        if(bind(*sock, (struct sockaddr *) &server_address, sizeof(server_address)) != 0){
            puts("E. Binding socket");
            return NULL;
        }
    }

    return (void *)sock;
}

// Receive data
int recvS(void *sock, char buffer[], int buff_size){
    if(recv(*(int *) sock, buffer, buff_size, 0) == -1){
        puts("E. Receiving");
        return 1;
    }
    return 0;
}

// Send data
int sendS(void *sock, char buffer[], int buff_size){
    if(send(*(int *) sock, buffer, buff_size, 0) == -1){
        puts("E. Sending");
        return 1;
    }
    return 0;
}

int listenS(void *sock){
    if(listen(*(int *) sock, 5) != 0){
        puts("E. Listening");
        return 1;
    }
    return 0;
}

void closeS(void *sock){
    close(*(int *) sock);
    free(sock);
}

void *acceptS(void *sock, char *ipOut, unsigned short *portOut){
    int *csock = (int *)malloc(sizeof(int));
    struct sockaddr_in client_addr;
    int len = sizeof(struct sockaddr_in);
    *csock = accept(*(int *) sock, (struct sockaddr *)&client_addr, (socklen_t*)&len);
    struct in_addr ipAddr = client_addr.sin_addr;
    //inet_ntop result 1 on success
    inet_ntop(AF_INET, &ipAddr, ipOut, INET_ADDRSTRLEN);
    *portOut = client_addr.sin_port;

    return (void *) csock;
}