#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include  "../../pkg/shared/consts.h"

void Loop(char *name, int socketFd);
void buildMessage(char *result, char *name, char *msg);
void setupAndConnect(struct sockaddr_in *serverAddr, struct hostent *host, int socketFd, long port);
void setNonBlock(int fd);
void interruptHandler(int sig);

static int socketFd;

int main(int argc, char *argv[])
{
    char *name;
    char *filename;
    struct sockaddr_in serverAddr;
    struct hostent *host;
    long port;

    if(argc != 4)
    {
        fprintf(stderr, "./client [username] [host] [port] \n");
        exit(1);
    }
    name = argv[1];
    if((host = gethostbyname(argv[2])) == NULL)
    {
        fprintf(stderr, "Couldn't get host name\n");
        exit(1);
    }
    port = strtol(argv[3], NULL, 0);
    if((socketFd = socket(AF_INET, SOCK_STREAM, 0))== -1)
    {
        fprintf(stderr, "Couldn't create socket\n");
        exit(1);
    }
    setupAndConnect(&serverAddr, host, socketFd, port);
    setNonBlock(socketFd);
    setNonBlock(0);

    //Set a handler for the interrupt signal
    signal(SIGINT, interruptHandler);

    Loop(name, socketFd);
}

//Main loop to take in input and display output result from server
void Loop(char *name, int socketFd)
{
    fd_set clientFds;

    while(1)
    {
        //Reset the fd set each time since select() modifies it
        FD_ZERO(&clientFds);
        FD_SET(socketFd, &clientFds);
        FD_SET(0, &clientFds);
        if(select(FD_SETSIZE, &clientFds, NULL, NULL, NULL) != -1) //wait for an available fd
        {
            for(int fd = 0; fd < FD_SETSIZE; fd++)
            {
                if(FD_ISSET(fd, &clientFds))
                {
                    //receive data from server
                    if(fd == socketFd)
                    {
                        char msgBuffer[CONSTS MAX_BUFFER];
                        int numBytesRead = read(socketFd, msgBuffer, CONSTS MAX_BUFFER - 1);
                        msgBuffer[numBytesRead] = '\0';
                        printf("%s", msgBuffer);
                        memset(&msgBuffer, 0, sizeof(msgBuffer));
                    }
                    //read from keyboard (stdin) and send to server
                    else if(fd == 0)
                    {
                        char payloadBuffer[CONSTS MAX_BUFFER];
                        fgets(payloadBuffer, CONSTS MAX_BUFFER - 1, stdin);
                        if(strcmp(payloadBuffer, "/exit\n") == 0)
                            //Reuse the interruptHandler function to disconnect the client
                            interruptHandler(-1);
                        else
                        {
                            char payloadMessage[CONSTS MAX_BUFFER];
                            buildMessage(payloadMessage, name, payloadBuffer);
                            if(write(socketFd, payloadMessage, CONSTS MAX_BUFFER - 1) == -1)
                            {
                                perror("write failed: ");
                            }
                            memset(&payloadBuffer, 0, sizeof(payloadBuffer));
                        }
                    }
                }
            }
        }
    }
}

//Concatenates the name with the message and puts it into result
void buildMessage(char *result, char *name, char *msg)
{
    memset(result, 0, CONSTS MAX_BUFFER);
    // strcpy(result, name);
    // strcat(result, ": ");
    strcat(result, msg);
}

//Sets up the socket and connects
void setupAndConnect(struct sockaddr_in *serverAddr, struct hostent *host, int socketFd, long port)
{
    memset(serverAddr, 0, sizeof(serverAddr));
    serverAddr->sin_family = AF_INET;
    serverAddr->sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    serverAddr->sin_port = htons(port);
    if(connect(socketFd, (struct sockaddr *) serverAddr, sizeof(struct sockaddr)) < 0)
    {
        perror("Couldn't connect to server");
        exit(1);
    }
}

//Sets the fd to nonblocking
void setNonBlock(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if(flags < 0)
        perror("fcntl failed");

    flags |= O_NONBLOCK;
    fcntl(fd, F_SETFL, flags);
}

//Notify the server when the client exits by sending "/exit"
void interruptHandler(int sig_unused)
{
    if(write(socketFd, "/exit\n", CONSTS MAX_BUFFER - 1) == -1)
        perror("write failed: ");

    close(socketFd);
    exit(1);
}