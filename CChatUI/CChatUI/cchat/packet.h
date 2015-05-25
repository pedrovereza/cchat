#pragma once

#define ALIAS_LEN 32
#define MESSAGE_LEN 140
#define OPTIONS_LEN 16

struct PACKET {
    char option[OPTIONS_LEN];
    char senderAlias[ALIAS_LEN];
    char message[MESSAGE_LEN];
};
