#include <stdio.h>

#pragma once

#define ALIAS_LEN 32

#define MAX_USERS 10

struct USER {
    pthread_t threadInfo;
    int socketfd;
    char alias[ALIAS_LEN];
};

struct NODE {
    struct USER user;
    struct NODE *next;
};

struct CHANNEL {
    struct NODE *head, *tail;
    int size;
    
    pthread_mutex_t channel_mutex;
};

int compare(struct USER *a, struct USER *b) {
    return a->socketfd - b->socketfd;
}

void channel_init(struct CHANNEL *channel) {
    channel->head = channel->tail = NULL;
    channel->size = 0;
    pthread_mutex_init(&channel->channel_mutex, NULL);
}

int channel_insert(struct CHANNEL *channel, struct USER *user) {
    if(channel->size == MAX_USERS)
        return -1;
    
    if(channel->head == NULL) {
        channel->head = (struct NODE *)malloc(sizeof(struct NODE));
        channel->head->user = *user;
        channel->head->next = NULL;
        channel->tail = channel->head;
    }
    else {
        channel->tail->next = (struct NODE *)malloc(sizeof(struct NODE));
        channel->tail->next->user = *user;
        channel->tail->next->next = NULL;
        channel->tail = channel->tail->next;
    }
    
    channel->size++;
    return 0;
}

int channel_delete(struct CHANNEL *channel, struct USER *user) {
    struct NODE *curr, *temp;
    
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
    
    for(curr = channel->head; curr->next != NULL; curr = curr->next) {
        if(compare(user, &curr->next->user) == 0) {
            temp = curr->next;
            if(temp == channel->tail) channel->tail = curr;
            curr->next = curr->next->next;
            free(temp);
            channel->size--;
            return 0;
        }
    }
    return -1;
}
