// THIS IS A FKING SERVER FOR LINUX GRRRRRRRR WOULD HAVE MADE IT FOR WINDOWS BUT THESE FKING THREAD
#include <mysql.h>
#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include <time.h>
#include<stdlib.h>
#include<unistd.h>
#include "Encryption/sha256.h"
#include "Linux/socket.h"

// Struct that store thread info
struct Thread{
    pthread_t ptid;
    time_t start;
    time_t last_msg;
    short admin;
};

void sha256_encode(char *text, int len)
{
	SHA256_CTX ctx;
	int idx;
	sha256_init(&ctx);
	sha256_update(&ctx, text, len);
	sha256_final(&ctx, text);
}

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
    MYSQL *mysqlCon;
};


//  The function seperate the username and the password from a received string
int seperatePasswordAndUser(char *username_and_password, char* username, char* password){
    int i, f = 0;
    for(i=0;f==0 && i<=12;i++){
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
    for(f=0;(f!=-1 && f<=32 && i<=44);i++, f++){
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
    // Get the variable passed to the function and store them
    char ip[16] = {0};
    unsigned short port = ((struct Connection_handler_struct *)clientArgs)->port;
    struct ThreadNode* curNode = ((struct Connection_handler_struct *)clientArgs)->curNode;
    strcpy(ip, ((struct Connection_handler_struct *)clientArgs)->ip);
    int clientSocketId = ((struct Connection_handler_struct *)clientArgs)->clientSocketId;
    MYSQL *mysqlCon = ((struct Connection_handler_struct *)clientArgs)->mysqlCon;

    // Free the struct passed to the function
    free(clientArgs);

    // Define time of the last interraction
    ((struct ThreadNode*)curNode)->currentThread.last_msg = time(NULL);
    printf("I-New connection. Ip:%s, Port:%d\n", ip, port);

    // Allocate 44 bytes for the username and password
    char username_and_password[45] = {0};

    // **The following lines can be optimised using malloc**
    char username[13] = {0};
    unsigned char password[33] = {0};
    while(1==1){
        recvS((void *)&clientSocketId, username_and_password, sizeof(username_and_password));
        if(seperatePasswordAndUser(username_and_password, username, password) != 0){
            //Exit username of password too long
            // Exit thread
        }
        char message[] = "Welcome to the server";
        printf("---Password: %s---\n", password);
        sha256_encode(password, strlen(password));
        printf("Username: %s", username);
        printf("---Password: %s---\n", password);
        int i = 0;
        while(i<32){
            printf("-%d-", password[i]);
            i++;
        }
        char *queryConn = (char *)malloc(200);
        sprintf(queryConn, "SELECT `UserId`, `Type` FROM `Account` WHERE `User`='%s' AND `Password`='%s'", username, password);
        if (mysql_query(mysqlCon, queryConn))
        {
            mysql_close(mysqlCon);
            exit(1);
        }
        free(queryConn);

        MYSQL_RES *result = mysql_store_result(mysqlCon);
        if (result == NULL)
        {
            printf("I-%s, tried to connect to user:%s and failed\n", ip, username);
            break;
            //finish_with_error(con);
        }


        int num_fields = mysql_num_fields(result);
        printf("---%d----\n", num_fields);
        
        MYSQL_ROW row;
        row = mysql_fetch_row(result);
        if(row == NULL){
            printf("I-%s, tried to connect to user:%s and failed\n", ip, username);
            break;
        }
        printf("---%s---\n", row[0]);
        sendS((void *)&clientSocketId, message, sizeof(message));
        recvS((void *)&clientSocketId, message, sizeof(message));
        printf("Message from %s: %s\n", ip, message);
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

    // This function should never stop until exit of the program
    while(1){
        // Time that will elpaps after each iteration
        sleep(60);

        // Set curNode back to the first node
        curNode=(struct ThreadNode *)startThreadNode;

        // Verify there is not only the default thread
        if(curNode->next_thread != NULL){
            current = time(NULL);
            curNode = curNode->next_thread;
            printf("D-Trying to find inactive connection...\n", curNode->currentThread.ptid);

            // Iterate until there is a Null pointer
            while(curNode->next_thread != NULL){
                if(curNode->currentThread.admin != 1 && difftime(current, curNode->currentThread.last_msg) >= 30){
                    temp = curNode->before_thread;
                    // Modify the previous node and the next one
                    curNode->next_thread->before_thread = curNode;
                    curNode->next_thread->before_thread = curNode;
                    printf("D-Ending thread %d...\n", curNode->currentThread.ptid);

                    // End thread and look up for error to log them
                    rc = pthread_cancel(curNode->currentThread.ptid);
                    if(rc){
                        printf("D-Failed to cancel the thread %d\n", curNode->currentThread.ptid);
                    }else{
                        printf("D-Thread %d has ended\n", curNode->currentThread.ptid);
                    }
                    
                    // Free curNode
                    free(curNode);

                    //Set curNode one node back
                    curNode = temp;
                }
            }
            printf("D-Inactive connection removed\n");
        }
    }
    
}


int main(){
    //Connect to mysql database
    MYSQL *mysqlCon = mysql_init(NULL);

    if (mysqlCon == NULL)
    {
        printf("Error while initialising variable mysqlCon\n");
    }

    if (mysql_real_connect(mysqlCon, "localhost", "test", "123",
            "Main", 0, NULL, 0) == NULL)
    {
        printf("Can't connect to mysql server\n");
        mysql_close(mysqlCon);
    }

    //Define threat related variable
    struct ThreadNode* startThreadNode = malloc(sizeof(struct ThreadNode));
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
            struct ThreadNode* clientNode = calloc(1, sizeof(struct ThreadNode));
            struct Connection_handler_struct* clientArgs = malloc(sizeof(struct Connection_handler_struct));
            clientArgs->port = *port;
            clientArgs->curNode = clientNode;
            clientArgs->clientSocketId = *((int *)clientSocketId);
            clientArgs->mysqlCon = mysqlCon;
            strcpy(clientArgs->ip, ip);
            err = pthread_create(&(clientNode->currentThread.ptid), NULL, &connection_handler, (void *)clientArgs);
            
            if (err != 0){
                printf("\nD-Can't create thread :[%s]\n", strerror(err));
                // Free clientNode and clientArgs
                free(clientArgs);
                free(clientNode);
            }else{
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