// THIS IS A FKING SERVER FOR LINUX GRRRRRRRR WOULD HAVE MADE IT FOR WINDOWS BUT THESE FKING THREAD

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <time.h>
#include<stdlib.h>
#include<unistd.h>
#include "Linux/socket.h"

struct Thread{
    pthread_t ptid;
    time_t start;
    time_t last_msg;
    short admin;
};

struct ThreadNode{
    struct ThreadNode* before_thread;
    struct Thread currentThread;
    struct ThreadNode* next_thread;
};


void* connection_handler(void* currThread){
    printf("\n Thread created successfully\n");
    puts("Connection accepted");
    ((struct Thread*)currThread)->last_msg = time(NULL);
    while(1==1){
        sleep(1);
        printf("a");
    }
    printf("bbbbbbbbbbb");
    //Reply to the client
    //sendS(csock, message, sizeof(message));
    //recvS(csock, message, sizeof(message));
    //printf("%s", message);
    return NULL;
}


void* garbage_collector_threat(void* startThreadNode){
    // Define variable
    struct ThreadNode *temp;
    struct ThreadNode *curNode;
    int rc = 0;
    time_t current;
    while(1){
        // Time that will elpaps after each iteration
        sleep(1);
        curNode=(struct ThreadNode *)startThreadNode;
        // Verify there is not only the default thread
        if(curNode->next_thread != NULL){
            current = time(NULL);
            curNode = curNode->next_thread;
            // Use this condition to iterate until there is a Null pointer
            while(curNode->next_thread != NULL){
                if(curNode->currentThread.admin != 1 && difftime(current, curNode->currentThread.last_msg) >= 5){
                    temp = curNode->before_thread;
                    // Modify the previous node and the next one
                    curNode->next_thread->before_thread = curNode;
                    curNode->next_thread->before_thread = curNode;
                    printf("D-Ending thread %d...", curNode->currentThread.ptid);
                    // End thread and look up for error to log them
                    rc = pthread_cancel(curNode->currentThread.ptid);
                    if(rc){
                        printf("D-Failed to cancel the thread %d\n", curNode->currentThread.ptid);
                    }else{
                        printf("D-Thread %d has ended.", curNode->currentThread.ptid);
                    }
                    // Free the space and set curNode back
                    free(curNode);
                    curNode = temp;
                }
            }
            //Display number of thread
            printf("D-There is currently %d thread", curNode->currentThread.ptid);
        }
    }
    
}


int main(){
    //Define threat related variable
    struct ThreadNode* startThreadNode = (struct ThreadNode*)malloc(sizeof(struct ThreadNode));
    struct ThreadNode* endThread = startThreadNode;
    startThreadNode->currentThread.admin = 1;
    startThreadNode->before_thread = NULL;
    startThreadNode->next_thread = NULL;
    int err;
    // Define server related variable
    void *serverSocketId;
    void *clientSocketId;
    char ip[16];
    unsigned short *port = (unsigned short *)malloc(sizeof(short));
    //Start socket and listen
    serverSocketId = initS("192.168.2.116", 9666, 0);
    listenS(serverSocketId);
    //Start the function that filter inactive connection
    pthread_create(&(startThreadNode->currentThread.ptid), NULL, &garbage_collector_threat, (void *)(startThreadNode));
    while(1){
        if((clientSocketId = acceptS(serverSocketId, ip, port)) != NULL )
        {
            //Handle thread and socket
            struct ThreadNode* clientNode = (struct ThreadNode*)malloc(sizeof(struct ThreadNode));
            err = pthread_create(&(clientNode->currentThread.ptid), NULL, &connection_handler, (void *)&(clientNode->currentThread));
            if (err != 0){
                printf("\ncan't create thread :[%s]", strerror(err));
                // Free space clientNode took
                free(clientNode);
            }
            else{
                // Add a new node to the NodeList
                clientNode->before_thread = endThread;
                endThread->next_thread = clientNode;
                clientNode->next_thread = NULL;
                clientNode->currentThread.start = time(NULL);
                clientNode->currentThread.last_msg = time(NULL);
                // Modify endThread to point the last
                endThread = clientNode;
            }
        }
    }
    return 0;
}