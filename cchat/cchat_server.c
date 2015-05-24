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

struct CHANNEL defaultChannel;
struct CHANNELS channels;

int main() {
    int sockfd, newsockfd;
    
    socklen_t clilen;
    
    struct sockaddr_in serv_addr, client_addr;
    
    channel_init(&defaultChannel);
    strcpy(defaultChannel.alias, DEFAULT_CHANNEL_ALIAS);
    channels_init(&channels);
    
    channels_insert(&channels, &defaultChannel);
    
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
            if(defaultChannel.size == MAX_USERS) {
                fprintf(stderr, "Connection full, request rejected...\n");
                continue;
            }
            printf("Connection requested received...\n");
            
            struct USER user;
            user_init(&user);
            user.socketfd = newsockfd;
            
            strcpy(user.alias, "Anonymous");
            strcpy(user.channelAlias, DEFAULT_CHANNEL_ALIAS);
            
            channel_insert(&channels.head->channel, &user);
            
            pthread_create(&user.threadInfo, NULL, client_handler, (void *)&user);
        }
    }
    
    return 0;
}

void *client_handler(void *fd) {
    struct USER user = *(struct USER *)fd;
    struct PACKET packet;
    struct UNODE *curr;
    struct CNODE *channelIterator;
    size_t bytes, sent;
    
    while(1) {
        bytes = recv(user.socketfd, (void *)&packet, sizeof(struct PACKET), 0);
        if(!bytes) {
            fprintf(stderr, "Connection lost from [%d] %s...\n", user.socketfd, user.alias);
            
            pthread_mutex_lock(&defaultChannel.channel_mutex);
            channel_delete(&defaultChannel, &user);
            pthread_mutex_unlock(&defaultChannel.channel_mutex);
            
            break;
        }
        
        printf("[%d] %s %s %s %s\n", user.socketfd, packet.option, user.channelAlias, packet.senderAlias, packet.message);
        
        if(!strcmp(packet.option, "alias")) {
            printf("Set alias to %s\n", packet.senderAlias);
            
            pthread_mutex_lock(&defaultChannel.channel_mutex);
            for(curr = defaultChannel.head; curr != NULL; curr = curr->next) {
                if(compare(&curr->user, &user) == 0) {
                    strcpy(curr->user.alias, packet.senderAlias);
                    strcpy(user.alias, packet.senderAlias);
                    break;
                }
            }
            pthread_mutex_unlock(&defaultChannel.channel_mutex);
        }
        else if (!strcmp(packet.option, "create")) {
            pthread_mutex_lock(&channels.channels_mutex);
            
            if (channels_contains(&channels, packet.message)) {
                //error: trying to create a channel with an existing channel alias
            }
            else {
                struct CHANNEL newChannel;
                channel_init(&newChannel);
                strncpy(newChannel.alias, packet.message, ALIAS_LEN);
                channels_insert(&channels, &newChannel);
            }
            
            pthread_mutex_unlock(&channels.channels_mutex);

        }
        else if (!strcmp(packet.option, "join")) {
            pthread_mutex_lock(&channels.channels_mutex);
            channelIterator = channels_find(&channels, packet.message);
            struct CHANNEL *channelToJoin = &channelIterator->channel;
            
            channelIterator = channels_find(&channels, user.channelAlias);
            struct CHANNEL *channelToLeave = &channelIterator->channel;
            pthread_mutex_unlock(&channels.channels_mutex);
            
            pthread_mutex_lock(&channelToJoin->channel_mutex);
            channel_insert(channelToJoin, &user);
            pthread_mutex_unlock(&channelToJoin->channel_mutex);
            
            bzero(&user.channelAlias, ALIAS_LEN);
            strcpy(user.channelAlias, channelToJoin->alias);
            
            pthread_mutex_lock(&channelToLeave->channel_mutex);
            channel_delete(channelToLeave, &user);
            pthread_mutex_unlock(&channelToLeave->channel_mutex);
        }
        
        else if(!strcmp(packet.option, "send")) {
            pthread_mutex_lock(&channels.channels_mutex);
            channelIterator = channels_find(&channels, user.channelAlias);
            struct CHANNEL *currentChannel = &channelIterator->channel;
            pthread_mutex_unlock(&channels.channels_mutex);
            
            pthread_mutex_lock(&currentChannel->channel_mutex);

            for(curr = currentChannel->head; curr != NULL; curr = curr->next) {
                if(!compare(&curr->user, &user))
                    continue;
            
                struct PACKET spacket;
                memset(&spacket, 0, sizeof(struct PACKET));
                
                strcpy(spacket.option, "msg");
                strcpy(spacket.senderAlias, packet.senderAlias);
                strcpy(spacket.message, packet.message);
                sent = send(curr->user.socketfd, (void *)&spacket, sizeof(struct PACKET), 0);
            }
            pthread_mutex_unlock(&currentChannel->channel_mutex);
        }
        else if(!strcmp(packet.option, "exit")) {
            printf("[%d] %s has disconnected...\n", user.socketfd, user.alias);
            
            pthread_mutex_lock(&defaultChannel.channel_mutex);
            channel_delete(&defaultChannel, &user);
            pthread_mutex_unlock(&defaultChannel.channel_mutex);
            
            break;
        }
        else {
            fprintf(stderr, "Garbage data from [%d] %s...\n", user.socketfd, user.alias);
        }
    }
    
    close(user.socketfd);
    
    return NULL;
}