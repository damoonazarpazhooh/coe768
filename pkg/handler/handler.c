#include "handler.h"

//The "consumer" -- waits for the Queue to have messages then takes them out and broadcasts to clients
HANDLER void *RequestHandler(void *data)
{
    Multiplexer *mux = (Multiplexer *)data;
    while(1)
    {
        //Obtain lock and pop message from Queue when not empty
        pthread_mutex_lock((mux->Queue)->mutex);
        while((mux->Queue)->empty)
        {
            pthread_cond_wait((mux->Queue)->notEmpty, (mux->Queue)->mutex);
        }
        char* msg = QUEUE Pop(mux->Queue);
        pthread_mutex_unlock((mux->Queue)->mutex);
        pthread_cond_signal((mux->Queue)->notFull);
        fprintf(stderr, "recieved message: %s\n", msg);
        fprintf(stderr, "Broadcasting .... \n");
        for(int i = 0; i < mux->numClients; i++)
        {
            int socket = mux->clientSockets[i];
            ProtocolHandler("ChatProtocol",socket,msg);
        }
    }
}
HANDLER void ProtocolHandler(char* name,int socket,char* payload){
    fprintf(stderr, "PROTOCOL: [%s]\n", name);
    if(socket != 0 && write(socket, payload, CONSTS MAX_BUFFER - 1) == -1){
        perror("Socket write failed: ");
    }
}

HANDLER void *FileHandler(void *data)
{
    Multiplexer *chatData = (Multiplexer *)data;
    QUEUE Queue *q = chatData->Queue;
    int *clientSockets = chatData->clientSockets;
    char path[1024];
    int socketFd =  chatData->socketFd;
    strcpy( path,"/workspace/coe768/cmd/server/" );
    while(1)
    {
        //Obtain lock and pop message from Queue when not empty
        pthread_mutex_lock(q->mutex);
        while(q->empty)
        {
            pthread_cond_wait(q->notEmpty, q->mutex);
        }
        char* msg = QUEUE Pop(q);
        pthread_mutex_unlock(q->mutex);
        pthread_cond_signal(q->notFull);
        char *source = NULL;
        strcat( path, UTILS Trim(msg));
        fprintf(stderr, "[FILE Handler] opening file: %s\n", path);
        FILE *fp = fopen(path, "r");
        if (fp != NULL) {
        /* Go to the end of the file. */
            if (fseek(fp, 0L, SEEK_END) == 0) {
                /* Get the size of the file. */
                long bufsize = ftell(fp);
                if (bufsize == -1) { /* Error */ }

                /* Allocate our buffer to that size. */
                source = malloc(sizeof(char) * (bufsize + 1));

                /* Go back to the start of the file. */
                if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }

                /* Read the entire file into memory. */
                size_t newLen = fread(source, sizeof(char), bufsize, fp);
                if ( ferror( fp ) != 0 ) {
                    fputs("Error reading file", stderr);
                } else {
                    source[newLen++] = '\0'; /* Just to be safe. */

                }
            }
        fclose(fp);
        fprintf(stderr, "file Content  %s\n",source);
        }else{
            fprintf(stderr, "file not found\n");
        }
        fprintf(stderr, "replying .... \n");
        for(int i = 0; i < chatData->numClients; i++)
        {
            int socket = clientSockets[i];
            // int socket = socketFd;
            if(socket != 0 && write(socket, msg, CONSTS MAX_BUFFER - 1) == -1)
                perror("Socket write failed: ");
        }
    }
}