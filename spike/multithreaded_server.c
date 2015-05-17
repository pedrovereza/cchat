#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 4000
#define MAX_NUMBER_OF_THREADS 3

pthread_t threads[MAX_NUMBER_OF_THREADS];


void* connectionHandler(void* socket_descriptor) {
    
    long numberOfBytes;
    char buffer[256];
    int socket = *(int*)socket_descriptor;
    
    bzero(buffer, 256);
    
    printf("Creating new thread to process...\n");
    
    /* read from the socket */
    numberOfBytes = read(socket, buffer, 256);
    if (numberOfBytes < 0)
        printf("ERROR reading from socket");
    printf("Here is the message: %s\n", buffer);
    
    /* write in the socket */
    numberOfBytes = write(socket,"Server: got your message", 24);
    if (numberOfBytes < 0)
        printf("ERROR writing to socket");
    
    close(socket);
    
    return NULL;
}

int main(int argc, char *argv[])
{
    int sockfd, newsockfd;
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, cli_addr;
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("ERROR on binding");
    
    listen(sockfd, MAX_NUMBER_OF_THREADS);
    
    clilen = sizeof(struct sockaddr_in);
    
    int i;
    for (i = 0; i < MAX_NUMBER_OF_THREADS; ++i) {
        
        if ((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
            printf("ERROR on accept");
        
        pthread_create(&threads[i], NULL, connectionHandler, &newsockfd);
    }
    
    for (i = 0; i < MAX_NUMBER_OF_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    close(sockfd);
    return 0; 
}