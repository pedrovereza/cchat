#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#include "packet.h"

#define PORT 4000

pthread_t sender_thread;
pthread_t listener_thread;

char currentAlias[ALIAS_LEN];

void *sender(void *args) {
    
    int sockfd = *(int *)args;
    
    char temp[MESSAGE_LEN];
    
    long numberOfBytes;
    
    while (1) {
        
        printf("Enter the message: ");
        
        struct PACKET packet;
        memset(&packet, 0, sizeof(struct PACKET));
        strcpy(packet.senderAlias, currentAlias);
        
        fgets(temp, MESSAGE_LEN, stdin);
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

void *listener(void *args) {
    
    int sockfd = *(int *)args;
    long numberOfBytes;
    
    struct PACKET packet;
    
    
    while (1) {
        bzero(packet.message, MESSAGE_LEN);
        bzero(packet.option, OPTIONS_LEN);
        bzero(packet.senderAlias, ALIAS_LEN);
        
        numberOfBytes = recv(sockfd, (void *)&packet, sizeof(struct PACKET), 0);
        
        
        if (numberOfBytes < 0)
            printf("ERROR reading from socket\n");
        
        write(1, packet.option, OPTIONS_LEN);
        
        if (!strcmp(packet.option, "alias-ok")) {
            bzero(&currentAlias, ALIAS_LEN);
            strcpy(currentAlias, packet.message);
        }
        else if (!strcmp(packet.option, "msg")) {
            write(1, packet.senderAlias, ALIAS_LEN);
            write(1," said:", 7);
            write(1, packet.message, MESSAGE_LEN);
            write(1, "\n", 1);
        }
    }
    
    return NULL;
}

int main(int argc, char *argv[])
{
    int sockfd;
    bzero(&currentAlias, ALIAS_LEN);
    strcpy(currentAlias, "Anonymous");
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    
    if (argc < 2) {
        fprintf(stderr,"usage %s hostname\n", argv[0]);
        exit(0);
    }
    
    server = gethostbyname(argv[1]);
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
    
    pthread_create(&sender_thread, NULL, sender, &sockfd);
    pthread_create(&listener_thread, NULL, listener, &sockfd);
    
    pthread_join(sender_thread, NULL);
    
    close(sockfd);
    
    return 0;
}