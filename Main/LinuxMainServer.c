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
    time_t last_msg;
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


// Struct used as argument for garbage_collector
struct Garbage_collector_struct{
    struct ThreadNode* startNode;
    MYSQL *mysqlCon;
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
void exitThreadAndConn(struct ThreadNode *curNode, char doExit, MYSQL *mysqlCon){
    // Modify the previous node and the next one
    if(curNode->next_node != NULL){
        (curNode->next_node)->before_node = curNode->before_node;
    }
    (curNode->before_node)->next_node = curNode->next_node;

    // Remove the connection from the ActiveConnection mysql table
    char mysqlQuery[70];
    // Perpare the mysql query
    sprintf(mysqlQuery, "DELETE FROM `ActiveConnection` WHERE `clientSocketId` = %d", curNode->currentThread.socketId);

    // Execute the query and exit if there is an error
    if (mysql_query(mysqlCon, mysqlQuery))
    {
        printf("E-Error with the mysql server\n");
    }

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
        if(wholeString[i] == '\n'){
            wholeString[i] = '\0';
            f=1;
        }else if(wholeString[i] == '\0'){
            return 1;
        }
    }
    // Quit, no newline detected
    if(f!=1){
        return 2;
    }
    *firstOut = wholeString;
    *secondOut = &(wholeString[i]);
    f=0; 
    do{
        f++;
    }while(wholeString[i+f] != '\0' && f<=secondStringSize);
    
    if(f>secondStringSize){
        return 3;
    }else{
        return 0;
    }
    
}


// Return the first row of a mysql query
MYSQL_ROW simpleMysqlQuery(MYSQL *mysqlCon, char *mysqlQuery){

    // Execute the query and exit if there is an error
    if (mysql_query(mysqlCon, mysqlQuery))
    {
        printf("E-Error with the mysql server\n");
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(mysqlCon);

    if (result == NULL)
    {
        printf("I-Can't find the element\n");
        return NULL;
    }

    MYSQL_ROW row;
    row = mysql_fetch_row(result);

    // Exit if there is no user with this password or username
    if(row == NULL){
        printf("I-There is no row with this name\n");
        return NULL;
    }
    
    // Return user's permission and convert the string -> int
    return row;
}


// This function authenticate the user and log the connection
int auth_client(char username_and_password[], char **username, char **password, char *ip, MYSQL *mysqlCon, char *mysqlQuery){
        // Seperate the user and the password from the string
        if(seperateTwoArgs(username_and_password, username, 13, password, 33) != 0){
            printf("I-The string %s sent is wrongly formated\n", ip);
            return -1;
        }

        // Hash the password
        sha256_hash(password, strlen(*password));

        // Perpare the mysql query
        sprintf(mysqlQuery, "SELECT `Type` FROM `Account` WHERE `User`='%s' AND `Password`='%s'", *username, *password);

        // Execute the query and get the result
        MYSQL_ROW row;
        row = simpleMysqlQuery(mysqlCon, mysqlQuery);

        if(row == NULL){
            return 0;
        }

        // Return user's permission and convert the string -> int
        return atoi(row[0]);
}


// This function execute a command and return the output
int exec_command(char *in, char *out, int size){
    FILE *fp = NULL;
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
    // Set the ip and port in the function
    char ip[16] = {0};
    strcpy(ip, ((struct Connection_handler_struct *)clientArgs)->ip);
    unsigned short port = ((struct Connection_handler_struct *)clientArgs)->port;

    // Set the other variable
    struct ThreadNode* curNode = ((struct Connection_handler_struct *)clientArgs)->curNode;
    int clientSocketId = ((struct Connection_handler_struct *)clientArgs)->clientSocketId;
    MYSQL *mysqlCon = ((struct Connection_handler_struct *)clientArgs)->mysqlCon;
    
    struct ThreadNode* startNode = ((struct Connection_handler_struct *)clientArgs)->startNode;

    // Free the struct passed to the function
    free(clientArgs);

    char mysqlQuery[256] = {0};

    // Define time of the last interraction
    ((struct ThreadNode*)curNode)->currentThread.last_msg = time(NULL);

    printf("I-New connection. Ip:%s, Port:%d\n", ip, port);

    // Allocate 45 bytes for the username and password
    char username_and_password[45] = {0};
    char *username = NULL;
    char *password = NULL;

    // Receive the username and the password
    if(recvS((void *)&clientSocketId, username_and_password, sizeof(username_and_password)) == 1){
        exitThreadAndConn(curNode, 1, mysqlCon);
        return NULL;
    }

    // Authenticate the client and get his permission
    int status = auth_client(username_and_password, &username, &password, ip, mysqlCon, mysqlQuery);
    if(status<0){
        printf("Exit...\n");
        exitThreadAndConn(curNode, 1, mysqlCon);
        return NULL;
    }

    printf("I-%s connected to the user:%s successfully\n", ip, username);

    // Perpare the mysql query
    sprintf(mysqlQuery, "INSERT INTO `ActiveConnection`(`clientSocketId`, `User`, `ip`, `port`) VALUES (%d, '%s', '%s', %d)", clientSocketId, username, ip, port);

    // Execute the query and exit if there is an error
    if (mysql_query(mysqlCon, mysqlQuery))
    {
        printf("E-Error with the mysql server\n");
    }
    
    // Send a success message   
    char message[] = "Welcome to the server, you are now connected!";
    sendS((void *)&clientSocketId, message, sizeof(message));

    // in is for the received string and out is for the output
    char in[256] = {0};
    char *firstArg = NULL;
    char *secondArg = NULL;
    char out[1024] = {0};
    
    // Listen for command and request
    while(1==1){
        command_wait:

        // 1000 * nb of microsecond(1/2 a second)
        usleep(500000);

        // Exit if an error occured in the receive function
        if(recvS((void *)&clientSocketId, in, sizeof(in)) == 0){
            // Set last interraction time
            ((struct ThreadNode *)curNode) ->currentThread.last_msg = time(NULL);

            // Perpare the mysql query
            sprintf(mysqlQuery, "SELECT `status` FROM `ActiveConnection` WHERE `clientSocketId` = '%d'", clientSocketId);

            // Get the status
            MYSQL_ROW row;
            row = simpleMysqlQuery(mysqlCon, mysqlQuery);

            if(row == NULL){
                printf("I-Row is NULL\n");
                break;
            }

            // Return user's permission and convert the string -> int
            status = atoi(row[0]);

            // Split the string sent in two arg
            if(seperateTwoArgs(in, &firstArg, 2, &secondArg, 254) != 0){
                if(sendS((void *)&clientSocketId, "Error, can't seperate the two argument\n", 39) != 0){
                    break;
                }
                goto command_wait;
            }

            // the char must be between 32 and 255
            printf("---%d---", *firstArg);

            // Do a specific task depending on the first argument the client sent
            switch (*firstArg)
            {

            // Execute a command and send back the output
            case 'a':
                printf("I-Executing command from %s: %s\n", ip, secondArg);

                // Execute the command and send the output
                if(exec_command(secondArg, out, sizeof(out)) == 0){
                    sendS((void *)&clientSocketId, out, strlen(out));
                    break;
                }

                sendS((void *)&clientSocketId, "Error, there is no command with this name or there's no output", 63);
                break;

            // Send a list of all the active connection
            case 'b':

                sprintf(mysqlQuery, "SELECT `User`, `ip`, `status` FROM `ActiveConnection` LIMIT %d,%d", atoi(secondArg), atoi(secondArg)+10);
                printf("%s", mysqlQuery);

                // Execute the query and exit if there is an error
                if (mysql_query(mysqlCon, mysqlQuery))
                {
                    printf("E-Error with the mysql server\n");
                    return NULL;
                }

                MYSQL_RES *result = mysql_store_result(mysqlCon);

                if (result == NULL)
                {
                    strcpy(out, "Nothing");
                }else{
                    MYSQL_ROW row = NULL;
                    row = mysql_fetch_row(result);
                    if(row == NULL){
                        strcpy(out, "Cursed");
                    }else{
                        sprintf(out, "%s\n%s\n%d", row[0], row[1], row[2]);
                    }
                }
                printf("%s", out);
                sendS((void *)&clientSocketId, out, strlen(out));
                break;
            
            default:
                sendS((void *)&clientSocketId, "Error, command type is unknown", 31);
                break;
            }

        } else{
            break;
        }
    }

    exitThreadAndConn(curNode, 1, mysqlCon);

    return NULL;
}


// This funtion go through all thread and kill inactive ones
void* garbage_collector(void* garbage_collector_arg){
    // Assign the argument received to permanent variable
    struct ThreadNode *startThreadNode =((struct Garbage_collector_struct *)garbage_collector_arg)->startNode;
    MYSQL *mysqlCon = ((struct Garbage_collector_struct *)garbage_collector_arg)->mysqlCon;
    
    // Free the received struct
    free(garbage_collector_arg);

    // Define variable
    struct ThreadNode *curNode;
    int rc = 0;
    time_t curTime;

    // This function should never stop until exit of the program
    while(1){
        // Time that will elpaps after each iteration(7 minutes)
        sleep(420);

        // Set curNode back to the first node
        curNode = startThreadNode;

        // Verify there is not only the default thread
        if(curNode->next_node != NULL){
            curTime = time(NULL);
            printf("I-Trying to find inactive connection...\n", curNode->currentThread.ptid);

            // Iterate until there is a Null pointer
            while(curNode->next_node != NULL){
                curNode = curNode->next_node;
                if(difftime(curTime, curNode->currentThread.last_msg) >= 420){
                    curNode = curNode->before_node;
                    exitThreadAndConn(curNode->next_node, 0, mysqlCon);
                }
            }
        }
    }
    
}
// <3 <3 <3

// This function will be executed at the start
int main(){

    // Initialize the variable where the connection to mysql will be stored
    MYSQL *mysqlCon = mysql_init(NULL);
    if (mysqlCon == NULL)
    {
        printf("E-Error while initialising variable mysqlCon\n");
        return -1;
    }

    // Connect to the mysql server
    if (mysql_real_connect(mysqlCon, "localhost", "test", "123",
            "Main", 0, NULL, 0) == NULL)
    {
        printf("E-Can't connect to the mysql server\n");
        mysql_close(mysqlCon);
        return -2;
    }

    // Delete the last ActiveConnection
    if (mysql_query(mysqlCon, "DELETE FROM `ActiveConnection`"))
    {
        printf("E-Can't delete the last connections\n");
        return -3;
    }



    // Define thread related variable
    struct ThreadNode* startThreadNode = malloc(sizeof(struct ThreadNode));
    struct ThreadNode* endThread = startThreadNode;
    int err;

    // Define server side related variable
    void *serverSocketId;
    void *clientSocketId;
    char ip[16];
    unsigned short port;

    // Start socket and listen
    serverSocketId = initS("192.168.2.116", 9666, 0);
    listenS(serverSocketId);

    // Start a function that filter inactive connection as thread and define some variable
    startThreadNode->before_node = NULL;
    startThreadNode->next_node = NULL;
    struct Garbage_collector_struct *garbage_collector_arg = malloc(sizeof(struct Garbage_collector_struct));
    garbage_collector_arg->startNode = startThreadNode;
    garbage_collector_arg->mysqlCon = mysqlCon;
    pthread_create(&(startThreadNode->currentThread.ptid), NULL, &garbage_collector, (void *)(garbage_collector_arg));

    // Basic loop that allow connection and affect the to a thread
    while(1){
        if((clientSocketId = acceptS(serverSocketId, ip, &port)) != NULL )
        {
            // 1000 * nb of microsecond(1/2 a second)
            usleep(500000);
            
            // Handle thread and socket
            struct ThreadNode* clientNode = calloc(1, sizeof(struct ThreadNode));
            struct Connection_handler_struct* clientArgs = malloc(sizeof(struct Connection_handler_struct));
            clientArgs->port = port;
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

                // Set last msg time(Will be used to exit the thread if client is inactive)
                clientNode->currentThread.last_msg = time(NULL);

                // Set the client socketid(Used to close the socket when the thread exit)
                clientNode->currentThread.socketId = *((int *)clientSocketId);

                // Modify endThread to point the last node
                endThread = clientNode;
            }

        }
    }
    return 0;
}