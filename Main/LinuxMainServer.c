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

void* connection_handler(void* currThread){
    printf("\n Thread created successfully\n");
    puts("Connection accepted");
    ((struct thread*)currThread)->last_msg = time(NULL);
    while(1==1){
        sleep(1);
        printf("a");
    }
    printf("bbbbbbbbbbb");
    //Reply to the client
    //sendS(csock, message, sizeof(message));
    //recvS(csock, message, sizeof(message));
    //printf("%s", message);


    // Exit

    return NULL;
}


void* garbage_collector_threat(void* startThreadList){
    struct list *temp;
    struct list *curNode;
    int i = 0;
    int rc = 0;
    time_t current;
    while(1){
        sleep(1);
        curNode=(struct list *)startThreadList;
        current = time(NULL);
        while(curNode->next_thread != NULL){
            i++;
            printf("-");
            if(curNode->currentThread.admin != 1 && difftime(current, curNode->currentThread.last_msg) >= 5){
                temp = curNode->before_thread;
                (curNode->next_thread)->before_thread = curNode;
                (curNode->next_thread)->before_thread = curNode;
                printf("--%p--", curNode->next_thread);
                printf("Ending thread %d...", i);
                rc = pthread_cancel(curNode->currentThread.ptid);
                if(rc) printf("failed to cancel the thread\n");
                free(curNode);
                curNode = temp;
            }

            curNode = curNode->next_thread;
            
        }
        i=0;
    }
    
}


int main(){
    //Define threat related variable
    struct list* startThreadList = (struct list*)malloc(sizeof(struct list));
    struct list* endThread = startThreadList;
    startThreadList->currentThread.admin = 1;
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
            struct list* clientNode = (struct list*)malloc(sizeof(struct list));
            err = pthread_create(&(clientNode->currentThread.ptid), NULL, &connection_handler, (void *)&(clientNode->currentThread));
            if (err != 0){
                printf("\ncan't create thread :[%s]", strerror(err));
                free(clientNode);
            }
            else{
                clientNode->before_thread = endThread;
                endThread->next_thread = clientNode;
                clientNode->next_thread = NULL;
                clientNode->currentThread.start = time(NULL);
                clientNode->currentThread.last_msg = time(NULL);
                endThread = clientNode;
            }
        }
    }
    return 0;
}