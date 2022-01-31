// This file is make out of funtion for a computer running windows
// PS. Thanks to my antivirus and it's real time scanning REALLY healpful;

#include <winsock2.h>
#include <stdio.h>
#include <stdlib.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

// Initialise socket
void *initS(char ip[], int port, char b)
{  
    // Create the basic variable
    WSADATA WSAData;
    SOCKET *sock = (SOCKET *)malloc(sizeof(SOCKET));
    SOCKADDR_IN sin;
    // Init wsa
    WSAStartup(MAKEWORD(2,0), &WSAData);
    //Handle specifing the ip, port and address type
    sin.sin_addr.s_addr = inet_addr(ip);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    //Create the socket et estfges jsekl fisjehf jkse
    if((*sock = socket(AF_INET,SOCK_STREAM,0)) == INVALID_SOCKET)
	{
        puts("E. Definning the variable sock");
        return NULL;
	}
    // Handle if it's a server or client(use char cuz 1 byte)
    if(b==1){
        // Connect
        if(connect(*sock, (SOCKADDR *)&sin, sizeof(sin))!=0){
            puts("E. Connecting to server");
            return NULL;
        }
    }else{
        // Bind the server
        if(bind(*sock, (SOCKADDR *)&sin, sizeof(sin)) == SOCKET_ERROR)
        {
            puts("E. Binding sock");
            return NULL;
        }
    }

    return (void *)sock;
}

//Receive function
int recvS(void *sock, char buffer[], int buff_size){
    if(recv(*(SOCKET *)sock, buffer, buff_size, 0) == -1){
        puts("E. Receiving");
        return 1;
    }
    return 0;
}

//Send function
int sendS(void *sock, char buffer[], int buff_size){
    if(send(*(SOCKET *)sock, buffer, buff_size, 0) == -1){
        puts("E. Sending");
        return 1;
    }
    return 0;
}

void closeS(void *sock, char freeBool)
{
    closesocket(*(SOCKET *)sock);
    if(freeBool==1){
        free(sock);
    }
    WSACleanup();
}

int listenS(void *sock){
    if(listen(*(SOCKET *)sock, 0)!=0){
        puts("E. Listening");
        return 1;
    }
    return 0;
}

void *acceptS(void *sock, char *ipOut, unsigned short *portOut)
{
    SOCKET *csock = (SOCKET *) malloc(sizeof(SOCKET));
    SOCKADDR_IN csin;
    int val = 0;
    int sinsize = sizeof(csin);
    if((*csock = accept(*(SOCKET *)sock, (SOCKADDR *)&csin, &sinsize)) != INVALID_SOCKET ){
        ipOut = inet_ntoa(csin.sin_addr);
        // Ca donne de la grosse merde comme port, wtfffff
        // News: HOLY SHIT, ca marche!!! bruhhhhhhhhhhh 2 heure perdu, oups
        *portOut = ntohs(csin.sin_port);
        return (void *)csock;
    }
    return NULL;
}