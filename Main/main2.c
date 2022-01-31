#include "stdio.h"
#include "stdlib.h"
#include <time.h>
#include <windows.h>
#include <winsock2.h>
#include "Linux/socket.h"

int main(){
    void *allo;
    char message[] = "william\n123";
    char out[1024] = {0};
    allo = initS("192.168.2.116", 9666, 1);
    sendS(allo, message, sizeof(message));
    recvS(allo, out, sizeof(out));
    printf("From server: %s\n", out);
    
    sendS(allo, "ls *", 5);
    recvS(allo, out, sizeof(out));
    printf("From server: %s\n", out);
    Sleep(5);
    printf("dafsefse ");
    return 0;
}