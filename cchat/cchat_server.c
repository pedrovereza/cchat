#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "channel.h"
#include "packet.h"

#define PORT 4000

void *client_handler(void *fd);

struct CHANNEL channel1;

int main() {
    int sockfd, newsockfd;
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, client_addr;
    
    channel_init(&channel1);
    
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        printf("ERROR opening socket");
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    bzero(&(serv_addr.sin_zero), 8);
    
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        printf("ERROR on binding");
    
    listen(sockfd, MAX_USERS);
    
    while(1) {
        clilen = sizeof(struct sockaddr_in);
        if((newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t*)&clilen)) == -1) {
            fprintf(stderr, "accept() failed...\n");
            return -1;
        }
        else {
            if(channel1.size == MAX_USERS) {
                fprintf(stderr, "Connection full, request rejected...\n");
                continue;
            }
            printf("Connection requested received...\n");
            
            struct USER user;
            user.socketfd = newsockfd;
            
            strcpy(user.alias, "Anonymous");
            
            pthread_mutex_lock(&channel1.channel_mutex);
            channel_insert(&channel1, &user);
            pthread_mutex_unlock(&channel1.channel_mutex);
            
            pthread_create(&user.threadInfo, NULL, client_handler, (void *)&user);
        }
    }
    
    return 0;
}

void *client_handler(void *fd) {
    struct USER user = *(struct USER *)fd;
    struct PACKET packet;
    struct NODE *curr;
    size_t bytes, sent;
    
    while(1) {
        bytes = recv(user.socketfd, (void *)&packet, sizeof(struct PACKET), 0);
        if(!bytes) {
            fprintf(stderr, "Connection lost from [%d] %s...\n", user.socketfd, user.alias);
            
            pthread_mutex_lock(&channel1.channel_mutex);
            channel_delete(&channel1, &user);
            pthread_mutex_unlock(&channel1.channel_mutex);
            
            break;
        }
        
        printf("[%d] %s %s %s\n", user.socketfd, packet.option, packet.senderAlias, packet.message);
        
        if(!strcmp(packet.option, "alias")) {
            printf("Set alias to %s\n", packet.senderAlias);
            
            pthread_mutex_lock(&channel1.channel_mutex);
            for(curr = channel1.head; curr != NULL; curr = curr->next) {
                if(compare(&curr->user, &user) == 0) {
                    strcpy(curr->user.alias, packet.senderAlias);
                    strcpy(user.alias, packet.senderAlias);
                    break;
                }
            }
            pthread_mutex_unlock(&channel1.channel_mutex);
        }
        else if(!strcmp(packet.option, "send")) {
            pthread_mutex_lock(&channel1.channel_mutex);
            for(curr = channel1.head; curr != NULL; curr = curr->next) {
                if(!compare(&curr->user, &user))
                    continue;
                
                struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));
                
                strcpy(spacket.option, "msg");
                strcpy(spacket.senderAlias, packet.senderAlias);
                strcpy(spacket.message, packet.message);
                sent = send(curr->user.socketfd, (void *)&spacket, sizeof(struct PACKET), 0);
            }
            pthread_mutex_unlock(&channel1.channel_mutex);
        }
        else if(!strcmp(packet.option, "exit")) {
            printf("[%d] %s has disconnected...\n", user.socketfd, user.alias);
            
            pthread_mutex_lock(&channel1.channel_mutex);
            channel_delete(&channel1, &user);
            pthread_mutex_unlock(&channel1.channel_mutex);
            
            break;
        }
        else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", user.socketfd, user.alias);
        }
    }
    
    close(user.socketfd);
    
    return NULL;
}