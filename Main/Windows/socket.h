void *initS(char ip[], int port, char b);
int recvS(void *sock, char buffer[], int buff_size);
int sendS(void *sock, char buffer[], int buff_size);
int listenS(void *sock);
void closeS(void *sock);
void *acceptS(void *sock, char *ipOut, unsigned short *portOut);