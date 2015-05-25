#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include "cchat_client.h"

#define PORT 4000

pthread_t sender_thread;
pthread_t listener_thread;

char currentAlias[ALIAS_LEN];

int *inputPipe;

void *senderThread(void *args) {
    
    int sockfd = *(int *)args;
    
    char temp[MESSAGE_LEN];
    
    long numberOfBytes;
    
    while (1) {
        struct PACKET packet;
        memset(&packet, 0, sizeof(struct PACKET));
        strcpy(packet.senderAlias, currentAlias);
        

        bzero(temp, MESSAGE_LEN);
        
        read(inputPipe[0], temp, MESSAGE_LEN);


        temp[strcspn(temp, "\n")] = 0;
        
        if (!strncmp("/exit", temp, 5)) {
            strcpy(packet.option, "exit");
            send(sockfd, (void *)&packet, sizeof(struct PACKET), 0);
            return NULL;
        }
        else if (!strncmp("/alias", temp, 6)) {
            strcpy(packet.option, "alias");
            strncpy(packet.senderAlias, temp + 7, ALIAS_LEN);
        }
        else if (!strncmp("/join", temp, 5)) {
            strcpy(packet.option, "join");
            strncpy(packet.message, temp + 6, ALIAS_LEN);
        }
        else if (!strncmp("/create", temp, 7)) {
            strcpy(packet.option, "create");
            strncpy(packet.message, temp + 8, ALIAS_LEN);
        }
        else if (!strncmp("/leave", temp, 7)) {
            strcpy(packet.option, "leave");
        }
        else  {
            strcpy(packet.option, "send");
            strcpy(packet.message, temp);
        }


        numberOfBytes = send(sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        
        if (numberOfBytes < 0)
            printf("ERROR writing to socket\n");
    }
    
    return NULL;
}

void *listenerThread(void *args) {
    int sockfd = *(int *)args;
    
    struct PACKET packet;
    
    while (1) {
        memset(&packet, 0, sizeof(struct PACKET));
        
        recv(sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        
        if (!strcmp(packet.option, "alias-ok")) {
            bzero(&currentAlias, ALIAS_LEN);
            strcpy(currentAlias, packet.message);
        }
        else if (!strcmp(packet.option, "error")) {
            write(1, "Error from server: ", 19);
            write(1, packet.message, MESSAGE_LEN);
            write(1, "\n", 1);
        }
        else if (!strcmp(packet.option, "msg")) {
            write(1, packet.senderAlias, ALIAS_LEN);
            write(1,": ", 2);
            write(1, packet.message, MESSAGE_LEN);
        }
    }
    
    return NULL;
}

void run(int *pipe)
{
    inputPipe = pipe;
    
    int sockfd;
    bzero(&currentAlias, ALIAS_LEN);
    strcpy(currentAlias, "Anonymous");
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket\n");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr = *((struct in_addr *)server->h_addr);
    bzero(&(serv_addr.sin_zero), 8);
    
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        printf("ERROR connecting\n");
    
    pthread_create(&sender_thread, NULL, senderThread, &sockfd);
    pthread_create(&listener_thread, NULL, listenerThread, &sockfd);
    
    pthread_join(sender_thread, NULL);
    pthread_kill(listener_thread, 9);
    
    close(sockfd);
}