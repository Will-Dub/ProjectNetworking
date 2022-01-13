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

int seperatePasswordAndUser(char *username_and_password, char* username, char* password){
    int i, f = 0;
    for(i=0;f==0 && i<=19;i++){
        if(username_and_password[i] != '-'){
            username[i] = username_and_password[i];
        }else{
            f=1;
        }
    }
    if(f==0){
        return 1;
    }
    username[i] = '\0';
    for(f=0;(f!=-1 && f<=29 && i<=49);i++, f++){
        password[f] = username_and_password[i];
        if(username_and_password[i] == '\0'){
            f=-2;
        }
    }
    if(f<0){
        return 2;
    }
    return 0;
}


// Function that handle each connection
void* connection_handler(void* clientArgs){
    char ip[16] = {0};
    unsigned short port = ((struct Connection_handler_struct *)clientArgs)->port;
    struct ThreadNode* curNode = ((struct Connection_handler_struct *)clientArgs)->curNode;
    strcpy(ip, ((struct Connection_handler_struct *)clientArgs)->ip);
    int clientSocketId = ((struct Connection_handler_struct *)clientArgs)->clientSocketId;
    free(clientArgs);
    ((struct ThreadNode*)curNode)->currentThread.last_msg = time(NULL);
    printf("\nI-New connection. Ip:%s, Port:%d\n", ip, port);
    char message[] = "Welcome to the server";
    char username_and_password[50] = {0};
    // Can be optimised using malloc
    char username[20] = {0};
    char password[30] = {0};
    while(1==1){
        recvS((void *)&clientSocketId, username_and_password, sizeof(username_and_password));
        seperatePasswordAndUser(username_and_password, username, password);
        printf("Username: %s", username);
        printf("Password: %s", password);
        sendS((void *)&clientSocketId, message, sizeof(message));
        recvS((void *)&clientSocketId, message, sizeof(message));
        printf("Message from %s: %s", ip, message);
        sleep(1);
        break;
    }
    //closeS((void *)&clientSocketId);
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
        sleep(60);
        curNode=(struct ThreadNode *)startThreadNode;
        // Verify there is not only the default thread
        if(curNode->next_thread != NULL){
            current = time(NULL);
            curNode = curNode->next_thread;
            printf("D-Removing inactive connection...", curNode->currentThread.ptid);
            // Iterate until there is a Null pointer
            while(curNode->next_thread != NULL){
                if(curNode->currentThread.admin != 1 && difftime(current, curNode->currentThread.last_msg) >= 30){
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
            printf("D-Inactive connection removed");
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
            struct ThreadNode* clientNode = calloc(sizeof(struct ThreadNode));
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