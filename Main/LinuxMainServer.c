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
    int socketId;
    time_t start;
    time_t last_msg;
    short admin;
};


// Struct used as a node in a list
struct ThreadNode{
    struct ThreadNode* before_node;
    struct Thread currentThread;
    struct ThreadNode* next_node;
};


// Struct used as argument for connection_handler
struct Connection_handler_struct{
    char ip[16];
    unsigned short port;
    int clientSocketId;
    struct ThreadNode* curNode;
    MYSQL *mysqlCon;
    struct ThreadNode* startNode;
};


// This function hash the text and return it in the same pointer
void sha256_hash(char **text, int len)
{
	SHA256_CTX ctx;
	sha256_init(&ctx);
	sha256_update(&ctx, *text, len);
	sha256_final(&ctx, *text);
}


// This function is used to close the thread, remove it from the list and close the connection
void exitThreadAndConn(struct ThreadNode *curNode, char doExit){
    // Modify the previous node and the next one
    if(curNode->next_node != NULL){
        (curNode->next_node)->before_node = curNode->before_node;
    }
    (curNode->before_node)->next_node = curNode->next_node;

    // End the connection between the server and the client
    closeS((void *)&(curNode->currentThread.socketId), 0);

    // Check for error
    pthread_t ptid = curNode->currentThread.ptid;

    // Free curNode
    free(curNode);

    // Exit the thread
    // **TO CHANGE**
    pthread_detach(ptid);
    if(doExit == 1){
        pthread_exit(NULL);
    }
    //pthread_cancel(curNode->currentThread.ptid);

    return;
}


//  The function seperate two arguments
int seperateTwoArgs(char wholeString[], char** firstOut, int firstStringSize, char** secondOut, int secondStringSize){
    int i, f = 0;
    for(i=0;f==0 && i<firstStringSize;i++){
        printf("%c", wholeString[i]);
        if(wholeString[i] == '\n'){
            wholeString[i] = '\0';
            f=1;
        }
    }
    // Quit, no newline detected
    if(f!=1){
        return 1;
    }
    *firstOut = wholeString;
    *secondOut = &(wholeString[i]);
    f=0; 
    do{
        f++;
    }while(wholeString[i+f] != '\0' && f<=secondStringSize);
    
    if(f>secondStringSize){
        return 2;
    }else{
        return 0;
    }
    
}


// This function authenticate the user and log the connection
int auth_client(char username_and_password[], char **username, char **password, char *ip, MYSQL *mysqlCon){
        // Seperate the user and the password from the string
        if(seperateTwoArgs(username_and_password, username, 13, password, 33) != 0){
            printf("I-The string %s sent is wrongly formated\n", ip);
            return -1;
        }

        // Hash the password
        sha256_hash(password, strlen(*password));
        char queryCon[120] = {0};

        // Perpare the mysql query
        sprintf(queryCon, "SELECT `Type` FROM `Account` WHERE `User`='%s' AND `Password`='%s'", *username, *password);

        // Execute the query and exit if there is an error
        if (mysql_query(mysqlCon, queryCon))
        {
            printf("E-Error with the mysql server\n");
            return -2;
        }

        MYSQL_RES *result = mysql_store_result(mysqlCon);
        if (result == NULL)
        {
            printf("I-%s, tried to connect to user:%s and failed\n", ip, *username);
            return -3;
        }

        MYSQL_ROW row;
        row = mysql_fetch_row(result);

        

        // Exit if there is no user with this password or username
        if(row == NULL){
            printf("I-%s, tried to connect to user:%s and failed\n", ip, *username);
            return -3;
        }
        
        // Return user's permission and convert the string -> int
        return atoi(row[0]);
}


// This function execute a command and return the output
int exec_command(char *in, char *out, int size){
    FILE *fp;
    char line[128] = {0};
    fp = popen(in, "r");
    
    //Verify if the command have an output
    if(fp)
    {
        while (fgets(line, sizeof(line), fp) != NULL)
            strcat(out, line);
    }else{
        // Return, either an error or there is no output
        return 1;
    }
    // Close the file pointer
    pclose(fp);

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
    struct ThreadNode* startNode = ((struct Connection_handler_struct *)clientArgs)->startNode;

    // Free the struct passed to the function
    free(clientArgs);

    // Define time of the last interraction
    ((struct ThreadNode*)curNode)->currentThread.last_msg = time(NULL);
    printf("I-New connection. Ip:%s, Port:%d\n", ip, port);

    // Allocate 45 bytes for the username and password
    char username_and_password[45] = {0};
    char *username = NULL;
    char *password = NULL;

    // Receive the username and the password
    if(recvS((void *)&clientSocketId, username_and_password, sizeof(username_and_password)) == 1){
        printf("Quitting...");
        exitThreadAndConn(curNode, 1);
    }

    // Verify the string is not null
    if(username_and_password == ""){
        exitThreadAndConn(curNode, 1);
    }

    // Authenticate the client and get his permission
    int accountLevel = auth_client(username_and_password, &username, &password, ip, mysqlCon);
    if(accountLevel<0){
        printf("Exit...");
        exitThreadAndConn(curNode, 1);
        return NULL;
    }
    printf("I-%s connected to the user:%s successfully\n", ip, username);

    // Send a success message
    char message[] = "Welcome to the server, you are now connected!";
    sendS((void *)&clientSocketId, message, sizeof(message));

    // in is for the received string and out is for the output
    char in[256] = {0};
    char *firstArg = NULL;
    char *secondArg = NULL;
    char out[1024] = {0};
    /*
    while(1==1){
        if(recvS((void *)&clientSocketId, in, sizeof(in)) == 0){
            if(seperateTwoArgs(in, firstArg, 128, secondArg, 128) != 0){
                printf("!!!-%s--%s-!!!!!", firstArg, secondArg);
                // Set last interraction time
                ((struct ThreadNode*)curNode)->currentThread.last_msg = time(NULL);

                printf("I-Executing command from %s: %s\n", ip, in);
                // Execute the command and send the output
                if(exec_command(in, out, 1024) == 0){
                    sendS((void *)&clientSocketId, out, strlen(out));
                }else{
                sendS((void *)&clientSocketId, "An error occured or there is no output", 39);
                }
            }
            else{
                sendS((void *)&clientSocketId, "An error occured or there is no output", 39);
            }
        }else{
            // An error occured while receiving so the thread exit
            break;
        }
    }*/

    exitThreadAndConn(curNode, 1);
    
    
    //closeS((void *)&clientSocketId);
    return NULL;
}


// This funtion go through all thread and kill inactive ones
void* garbage_collector_threat(void* startThreadNode){
    // Define variable
    struct ThreadNode *curNode;
    int rc = 0;
    time_t current;

    // This function should never stop until exit of the program
    while(1){
        // Time that will elpaps after each iteration(7 minutes)
        sleep(420);

        // Set curNode back to the first node
        curNode=(struct ThreadNode *)startThreadNode;

        // Verify there is not only the default thread
        if(curNode->next_node != NULL){
            current = time(NULL);
            curNode = curNode->next_node;
            printf("I-Trying to find inactive connection...\n", curNode->currentThread.ptid);

            // Iterate until there is a Null pointer
            while(curNode != NULL){
                if(curNode->currentThread.admin != 1 && difftime(current, curNode->currentThread.last_msg) >= 420){
                    curNode = curNode->before_node;
                    exitThreadAndConn(curNode->next_node, 0);
                }
            }
            printf("I-Inactive connection removed\n");
        }
    }
    
}
// <3 <3 <3

// This function will be executed at the start
int main(){
    // Connect to mysql database
    MYSQL *mysqlCon = mysql_init(NULL);

    if (mysqlCon == NULL)
    {
        printf("E-Error while initialising variable mysqlCon\n");
    }

    if (mysql_real_connect(mysqlCon, "localhost", "test", "123",
            "Main", 0, NULL, 0) == NULL)
    {
        printf("E-Can't connect to the mysql server\n");
        mysql_close(mysqlCon);
    }

    // Define thread related variable
    struct ThreadNode* startThreadNode = malloc(sizeof(struct ThreadNode));
    struct ThreadNode* endThread = startThreadNode;
    int err;

    // Define server side related variable
    void *serverSocketId;
    void *clientSocketId;
    char ip[16];
    unsigned short *port = (unsigned short *)malloc(sizeof(short));

    // Start socket and listen
    serverSocketId = initS("192.168.2.116", 9666, 0);
    listenS(serverSocketId);

    // Start a function that filter inactive connection as thread and define some variable
    startThreadNode->currentThread.admin = 1;
    startThreadNode->before_node = NULL;
    startThreadNode->next_node = NULL;
    pthread_create(&(startThreadNode->currentThread.ptid), NULL, &garbage_collector_threat, (void *)(startThreadNode));

    // Basic loop that allow connection and affect the to a thread
    
    while(1){
        if((clientSocketId = acceptS(serverSocketId, ip, port)) != NULL )
        {
            // 1000 * nb of microsecond(1/2 a second)
            usleep(500000);
            
            // Handle thread and socket
            struct ThreadNode* clientNode = calloc(1, sizeof(struct ThreadNode));
            struct Connection_handler_struct* clientArgs = malloc(sizeof(struct Connection_handler_struct));
            clientArgs->port = *port;
            clientArgs->curNode = clientNode;
            clientArgs->clientSocketId = *((int *)clientSocketId);
            clientArgs->mysqlCon = mysqlCon;
            clientArgs->startNode = startThreadNode;
            strcpy(clientArgs->ip, ip);
            err = pthread_create(&(clientNode->currentThread.ptid), NULL, &connection_handler, (void *)clientArgs);

            // Handle all errors linked to the creation of the thread
            if (err != 0){
                printf("\nE-Can't create thread :[%s]\n", strerror(err));

                // Free clientNode and clientArgs
                free(clientArgs);
                free(clientNode);
                closeS(clientSocketId, 0);
            }else{
                // Add a new node to the NodeList
                clientNode->before_node = endThread;
                endThread->next_node = clientNode;
                clientNode->next_node = NULL;
                clientNode->currentThread.start = time(NULL);
                clientNode->currentThread.last_msg = time(NULL);
                clientNode->currentThread.socketId = *((int *)clientSocketId);

                // Modify endThread to point the last node
                endThread = clientNode;
            }

        }
    }
    return 0;
}