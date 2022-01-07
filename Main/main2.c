#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include<windows.h>
#include <winsock2.h>
#include "Linux/socket.h"
int main(){
    void *allo;
    char message[100] = {0};
    allo = initS("192.168.2.116", 9666, 1);
    printf("----");
    recvS(allo, message, 100);
    sendS(allo, message, 100);
    CreateThread
    printf("----");
    printf("%s", message);
    printf("----");
    return 0;
}