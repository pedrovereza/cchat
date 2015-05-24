#include <stdio.h>

#pragma once

#define ALIAS_LEN 32

#define MAX_USERS 10

#define DEFAULT_CHANNEL_ALIAS "default"

struct USER {
    pthread_t threadInfo;
    int socketfd;
    char alias[ALIAS_LEN];
    char channelAlias[ALIAS_LEN];
};

struct UNODE {
    struct USER user;
    struct UNODE *next;
};

struct CHANNEL {
    struct UNODE *head, *tail;
    int size;
    char alias[ALIAS_LEN];
    
    pthread_mutex_t channel_mutex;
};

struct CNODE {
    struct CHANNEL channel;
    struct CNODE *next;
};

struct CHANNELS {
    struct CNODE *head, *tail;
    int size;
    
    pthread_mutex_t channels_mutex;
};

void user_init(struct USER *user) {
    bzero(&user->alias, ALIAS_LEN);
    bzero(&user->channelAlias, ALIAS_LEN);
}

int compare_channels(struct CHANNEL *a, struct CHANNEL *b ) {
    return strcmp(a->alias, b->alias) == 0;
}

void channels_init(struct CHANNELS *channels) {
    channels->head = channels->tail = NULL;
    channels->size = 0;
    
    pthread_mutex_init(&channels->channels_mutex, NULL);
}

int channels_contains(struct CHANNELS *channels, char *alias) {
    struct CNODE *current;
    
    for (current = channels->head; current->next != NULL; current = current->next) {
        if (!strcmp(current->channel.alias, alias))
            return 1;
    }
    
    return 0;
}

struct CNODE* channels_find(struct CHANNELS *channels, char *alias) {
    struct CNODE *current;

    for (current = channels->head; current != NULL; current = current->next) {
        write(1, current->channel.alias, ALIAS_LEN);
        if (!strncmp(current->channel.alias, alias, ALIAS_LEN)) {
            return current;
        }
    }
    
    return NULL;
}

int channels_insert(struct CHANNELS *channels, struct CHANNEL *aChannel) {
    if(channels->head == NULL) {
        channels->head = (struct CNODE *)malloc(sizeof(struct CNODE));
        channels->head->channel = *aChannel;
        channels->head->next = NULL;
        channels->tail = channels->head;
    }
    else {
        channels->tail->next = (struct CNODE *)malloc(sizeof(struct CNODE));
        channels->tail->next->channel = *aChannel;
        channels->tail->next->next = NULL;
        channels->tail = channels->tail->next;
    }
    
    channels->size++;
    return 0;
}

int channels_delete(struct CHANNELS *channels, struct CHANNEL *aChannel) {
    struct CNODE *current, *temp;
    
    if(channels->head == NULL)
        return -1;
    
    if(compare_channels(aChannel, &channels->head->channel) == 0) {
        temp = channels->head;
        channels->head = channels->head->next;
        if(channels->head == NULL) channels->tail = channels->head;
        free(temp);
        channels->size--;
        return 0;
    }
    
    for(current = channels->head; current->next != NULL; current = current->next) {
        if(compare_channels(aChannel, &current->next->channel) == 0) {
            temp = current->next;
            
            if(temp == channels->tail)
                channels->tail = current;
            
            current->next = current->next->next;
            free(temp);
            
            channels->size--;
            return 0;
        }
    }
    return -1;
}


int compare(struct USER *a, struct USER *b) {
    return a->socketfd - b->socketfd;
}

void channel_init(struct CHANNEL *channel) {
    channel->head = channel->tail = NULL;
    channel->size = 0;
    
    bzero(&channel->alias, ALIAS_LEN);
    
    pthread_mutex_init(&channel->channel_mutex, NULL);
}

int channel_insert(struct CHANNEL *channel, struct USER *user) {
    if(channel->size == MAX_USERS)
        return -1;
    
    if(channel->head == NULL) {
        channel->head = (struct UNODE *)malloc(sizeof(struct UNODE));
        channel->head->user = *user;
        channel->head->next = NULL;
        channel->tail = channel->head;
    }
    else {
        channel->tail->next = (struct UNODE *)malloc(sizeof(struct UNODE));
        channel->tail->next->user = *user;
        channel->tail->next->next = NULL;
        channel->tail = channel->tail->next;
    }
    
    channel->size++;
    return 0;
}

int channel_delete(struct CHANNEL *channel, struct USER *user) {
    struct UNODE *current, *temp;
    
    if(channel->head == NULL)
        return -1;
    
    if(compare(user, &channel->head->user) == 0) {
        temp = channel->head;
        channel->head = channel->head->next;
        if(channel->head == NULL) channel->tail = channel->head;
        free(temp);
        channel->size--;
        return 0;
    }
    
    for(current = channel->head; current->next != NULL; current = current->next) {
        if(compare(user, &current->next->user) == 0) {
            temp = current->next;
            
            if(temp == channel->tail)
                channel->tail = current;
            
            current->next = current->next->next;
            free(temp);
            
            channel->size--;
            return 0;
        }
    }
    return -1;
}
