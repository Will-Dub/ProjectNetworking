#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <pthread.h>
#include<windows.h>
#include "Linux/socket.h"

int connection_handler(void *csock, char ip[], unsigned short *port){

}

int main(){
    void *allo;
    void *hi;
    char ip[16];
    unsigned short *port = (unsigned short *)malloc(sizeof(short));
    char message[] = "Hello Client , I have received your connection. But I have to go now, bye\n";
    allo = initS("192.168.2.116", 9666, 0);
    listenS(allo);
    printf("allo123");
    while(1){
        if((hi = acceptS(allo, ip, port)) != NULL )
        {
            puts("Connection accepted");
            //Reply to the client
            sendS(hi, message, sizeof(message));
            recvS(hi, message, sizeof(message));
            printf("%s", message);
        }
    }
    return 0;
}


