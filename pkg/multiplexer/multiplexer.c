#include "multiplexer.h"
#include "../protocol/protocol.h"

//Thread to handle new connections. Adds client's fd to list of client fds and spawns a new ClientHandler thread for it
MULTIPLEXER void *Multiplex(void *data)
{
    Multiplexer *mux = (Multiplexer *) data;
    while(1)
    {
        int clientSocketFd = accept((mux->conn)->socketFd, NULL, NULL);
        if(clientSocketFd > 0)
        {
            fprintf(stderr, "MULTIPLEXER accepted new client. Socket: %d\n", clientSocketFd);
            //Obtain lock on clients list and add new client in
            pthread_mutex_lock(mux->clientListMutex);
            if((mux->conn)->numClients < CONSTS MAX_BUFFER)
            {
                //Add new client to list
                for(int i = 0; i < CONSTS MAX_BUFFER; i++)
                {
                    if(!FD_ISSET((mux->conn)->clientSockets[i], &(mux->readFds)))
                    {
                        (mux->conn)->clientSockets[i] = clientSocketFd;
                        i = CONSTS MAX_BUFFER;
                    }
                }

                FD_SET(clientSocketFd, &(mux->readFds));
                mux->clientSocketFd = clientSocketFd;

                pthread_t clientThread;
                if((pthread_create(&clientThread, NULL, (void *)&ClientHandler, (void *)mux)) == 0)
                {
                    (mux->conn)->numClients++;
                    fprintf(stderr, "Client connection to server has been successfully multiplexed on socket: %d\n", clientSocketFd);
                }
                else
                    close(clientSocketFd);
            }
            pthread_mutex_unlock(mux->clientListMutex);
        }
    }
}

//ClientHandler - Listens for payloads from client to add to queue
MULTIPLEXER void *ClientHandler(void *chv)
{
    Multiplexer *data = (Multiplexer *)chv;

    QUEUE Queue *q = data->Queue;
    int clientSocketFd = data->clientSocketFd;
    char buffer[CONSTS MAX_BUFFER];
    int read_size;
    while((read_size = recv(clientSocketFd, buffer, MAX_BUFFER, 0)) > 0)
    {

        // If the client sent /exit\n,
        // remove them from the client list and close their socket
        if(strcmp(buffer, "/exit\n") == 0)
        {
            fprintf(stderr, "Client on socket %d has disconnected.\n", clientSocketFd);
            Disconnect(data, clientSocketFd);
            return NULL;
        }
        else
        {
            //Wait for Queue to not be full before pushing message
            while(q->full)
            {
                pthread_cond_wait(q->notFull, q->mutex);
            }
            // extracting message length
            unsigned char data[ExtractMessageLength(buffer)];
            // extracting data
            ExtractMessageData(data,buffer);
            //Obtain lock, push payload to Queue, unlock, set condition variable
            pthread_mutex_lock(q->mutex);
            if (strlen(data) !=0){
                fprintf(stderr, "Pushing payload to Queue: %s\n", data);
                QUEUE Push(q,clientSocketFd, data);
            }
            pthread_mutex_unlock(q->mutex);
            pthread_cond_signal(q->notEmpty);
        }
    }
}

//The "consumer" -- waits for the Queue to have messages then takes them out and broadcasts to clients
MULTIPLEXER void *RequestHandler(void *data)
{
    MULTIPLEXER Multiplexer *mux = (Multiplexer *)data;
    while(1)
    {
        //Obtain lock and pop message from Queue when not empty
        pthread_mutex_lock((mux->Queue)->mutex);
        while((mux->Queue)->empty)
        {
            pthread_cond_wait((mux->Queue)->notEmpty, (mux->Queue)->mutex);
        }
        PAYLOAD Payload *message = QUEUE Pop(mux->Queue);
        pthread_mutex_unlock((mux->Queue)->mutex);
        pthread_cond_signal((mux->Queue)->notFull);
        fprintf(stderr, "recieved message data: %s\n" ,message->data);
        fprintf(stderr, "Broadcasting .... \n");
        HANDLER ProtocolHandler("Chat-v1",mux->conn,message);
    }
}

//Removes the socket from the list of active client sockets and closes it
MULTIPLEXER void Disconnect(Multiplexer *data, int clientSocketFd)
{
    pthread_mutex_lock(data->clientListMutex);
    for(int i = 0; i < CONSTS MAX_BUFFER; i++)
    {
        if((data->conn)->clientSockets[i] == clientSocketFd)
        {
            (data->conn)->clientSockets[i] = 0;
            close(clientSocketFd);
            (data->conn)->numClients--;
            i = CONSTS MAX_BUFFER;
        }
    }
    pthread_mutex_unlock(data->clientListMutex);
}
