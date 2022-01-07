// THIS IS A FKING SERVER FOR LINUX GRRRRRRRR WOULD HAVE MADE IT FOR WINDOWS BUT THESE FKING THREAD

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <time.h>
#include<stdlib.h>
#include<unistd.h>
#include "Linux/socket.h"

struct thread{
    pthread_t ptid;
    time_t start;
    time_t last_msg;
    short admin;
};

struct list{
    struct list* before_thread;
    struct thread currentThread;
    struct list* next_thread;
};

void* connection_handler(){
    printf("\n Thread created successfully\n");
    puts("Connection accepted");
    //Reply to the client
    //sendS(csock, message, sizeof(message));
    //recvS(csock, message, sizeof(message));
    //printf("%s", message);


    // Exit

    return NULL;
}


void* garbage_collector_threat(void* startThreadList){
    while(1==1){
        sleep(20);
        printf("This come from garbage func: %p;;;;;", ((struct list*)startThreadList)->next_thread);
    }
    
}


int main(){
    //Define threat related variable
    struct list* startThreadList = malloc(sizeof(struct list));
    struct list* endThread = startThreadList;
    startThreadList->before_thread = NULL;
    startThreadList->next_thread = NULL;
    int err;
    // Define server related variable
    void *socketId;
    void *csock;
    char ip[16];
    unsigned short *port = (unsigned short *)malloc(sizeof(short));
    //Start socket and listen
    socketId = initS("192.168.2.116", 9666, 0);
    listenS(socketId);
    //Start the function that filter inactive connection
    pthread_create(&(startThreadList->currentThread.ptid), NULL, &garbage_collector_threat, (void *)(startThreadList));
    while(1){
        if((csock = acceptS(socketId, ip, port)) != NULL )
        {
            //Handle thread and socket
            struct list* clientThread = malloc(sizeof(struct list));
            err = pthread_create(&(clientThread->currentThread.ptid), NULL, &connection_handler, NULL);
            if (err != 0){
                printf("\ncan't create thread :[%s]", strerror(err));
                free(clientThread);
            }
            else{
                clientThread->before_thread = endThread;
                endThread->next_thread = clientThread;
                clientThread->next_thread = NULL;
                clientThread->currentThread.start = time(NULL);
                clientThread->currentThread.last_msg = time(NULL);
                endThread = clientThread;
                printf("%p", clientThread);
            }
        }
    }
    return 0;
}