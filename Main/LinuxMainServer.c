// THIS IS A FKING SERVER FOR LINUX GRRRRRRRR WOULD HAVE MADE IT FOR WINDOWS BUT THESE FKING THREAD

#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <time.h>
#include<stdlib.h>
#include<unistd.h>
#include "Linux/socket.h"

// Struct that store thread info
struct Thread{
    pthread_t ptid;
    time_t start;
    time_t last_msg;
    short admin;
};

// Struct used as a node in a list
struct ThreadNode{
    struct ThreadNode* before_thread;
    struct Thread currentThread;
    struct ThreadNode* next_thread;
};

// Struct used as argument for connection_handler
struct Connection_handler_struct{
    char ip[16];
    unsigned short port;
    int clientSocketId;
    struct ThreadNode* curNode;
};


// Function that handle each connection
void* connection_handler(void* clientArgs){
    char ip[16] = {0};
    unsigned short port = ((struct Connection_handler_struct *)clientArgs)->port;
    struct ThreadNode* curNode = ((struct Connection_handler_struct *)clientArgs)->curNode;
    strcpy(ip, ((struct Connection_handler_struct *)clientArgs)->ip);
    int clientSocketId = ((struct Connection_handler_struct *)clientArgs)->clientSocketId;
    free(clientArgs);
    printf("\nD-Thread created successfully\n");
    printf("\nI-New connection. Ip:%s, Port:%d\n", ip, port);
    char message[] = "WELCOME!!!";
    while(1==1){
        sendS((void *)&clientSocketId, message, sizeof(message));
        recvS((void *)&clientSocketId, message, sizeof(message));
        printf("Message from %s: %s", ip, message);
        //((struct ThreadNode*)currNode)->last_msg = time(NULL);
        sleep(1);
        break;
    }
    return NULL;
}


// This funtion go through all thread and kill inactive ones
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
                    // Free curNode
                    free(curNode);
                    //Set curNode one node back
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
    int err;
    // Define server related variable
    void *serverSocketId;
    void *clientSocketId;
    char ip[16];
    unsigned short *port = (unsigned short *)malloc(sizeof(short));
    //Start socket and listen
    serverSocketId = initS("192.168.2.116", 9666, 0);
    listenS(serverSocketId);
    //Start a function that filter inactive connection as thread and define some variable
    startThreadNode->currentThread.admin = 1;
    startThreadNode->before_thread = NULL;
    startThreadNode->next_thread = NULL;
    pthread_create(&(startThreadNode->currentThread.ptid), NULL, &garbage_collector_threat, (void *)(startThreadNode));
    //Basic loop that allow connection and affect the to a thread
    while(1){
        if((clientSocketId = acceptS(serverSocketId, ip, port)) != NULL )
        {
            //Handle thread and socket
            struct ThreadNode* clientNode = malloc(sizeof(struct ThreadNode));
            struct Connection_handler_struct* clientArgs = malloc(sizeof(struct Connection_handler_struct));
            clientArgs->port = *port;
            clientArgs->curNode = clientNode;

            clientArgs->clientSocketId = *((int *)clientSocketId);
            strcpy(clientArgs->ip, ip);
            err = pthread_create(&(clientNode->currentThread.ptid), NULL, &connection_handler, (void *)clientArgs);
            if (err != 0){
                printf("\nD-Can't create thread :[%s]", strerror(err));
                // Free clientNode and clientArgs
                free(clientArgs);
                free(clientNode);
            }
            else{
                // Add a new node to the NodeList
                clientNode->before_thread = endThread;
                endThread->next_thread = clientNode;
                clientNode->next_thread = NULL;
                clientNode->currentThread.start = time(NULL);
                clientNode->currentThread.last_msg = time(NULL);
                // Modify endThread to point the last node
                endThread = clientNode;
            }
        }
    }
    return 0;
}