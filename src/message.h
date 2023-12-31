#ifndef __MESSAGE_DESCRIPTION__
#define __MESSAGE_DESCRIPTION__

#include "./def.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define CHECKSUM_SIZE 100
#define MAX_CONTENT_AT_ONCE 1024

typedef enum {
    E_ERROR = -1,
    E_HELLO = 0,
    E_HELLO_ACK,
    E_HEART_BEAT,
    E_TEXT_MESSGAE,
    E_CONTENT_CONTINUE,
    E_CONTENT_CONT_END,
    /* Below This are just a dream ;) */
    E_AUDIO_MESSAGE,
    E_VIDEO_MESSGAE,
    E_AV_MESSAGE,
    E_RDP_MESSAGE, // Remote Desktop and Screen Sharing and Casting
} MSG_TYPE;

typedef struct {
        /*
        type      {MSG_TYPE}
        sequence  {unsigned-integer denoting the communication count}
        length    {unsigned-integer denoting the length of MSG_DATA
                    [it doesnot count the headers stuff ie:
                    checksum and length itself]
                    }
        */
        MSG_TYPE type;
        uint32_t sequence;
        uint32_t length;
        char checksum[CHECKSUM_SIZE];
} MSG_HEADER;

typedef struct {
        CLIENT to;
        CLIENT from;
        struct timeval timestamp;
        union {
                char text[MAX_CONTENT_AT_ONCE]; // for TEXT_MESSAGE
                char
                    content[MAX_CONTENT_AT_ONCE]; // for AV_MESSGAES(Audio,Video
                                                  // and Both)
        };
} MSG_DATA;

typedef struct {
        MSG_HEADER header;
        MSG_DATA data;
} MESSAGE;

typedef struct {
        MSG_HEADER header;
        union {
                uint32_t ping;
                uint32_t pong;
        };
} M_HEARTBEAT;

typedef struct {
        MSG_HEADER header;
        union {
                char text[MAX_CONTENT_AT_ONCE];
                char content[MAX_CONTENT_AT_ONCE];
        };
} M_CONTENT_CONTINUE;

typedef struct {
        MSG_HEADER header;
        uint32_t acknowledged;
} M_HELLO_ACK;

typedef struct {
        MSG_HEADER header;
        union {
                uint32_t from_client;
                uint32_t from_server;
        };
} M_HELLO;

typedef struct {
        int sockfd;
        union {
                struct sockaddr_in sockaddr;
                struct sockaddr _sockaddr;
        };
        int *signal;
        CLIENT self;
} info;

typedef union {
        CLIENT client;
        SERVER server;
} CLIENT_SERVER;

typedef enum {
    READABLE = 0,
    WRITEABLE,
    READWRITE, // READABLE|WRITEABLE
} BUF_TYPE;
typedef enum {
    D_TEXT = 0,
    D_FILE,
    D_LIVE,
} DATA_TYPE;

typedef struct {
        BUF_TYPE type;
        DATA_TYPE dtype;
        union {
                char *message;
                FILE *file;
        };
} readable, writeable;

void error(const char *msg);
int has_data(int sock);
int send_hello(const info to, const CLIENT _client);
int recv_hello(const info __has_dot_sockfd, void *buffer);
int send_ack(const info to, uint32_t ack_value);
int recive_ack(const info __has_dot_sockfd, void *buffer);
int send_heart_beat(const info to);
int recv_heart_beat(const info __has_dot_sockfd, void *buffer);
int recv_text(const info __has_dot_sockfd, void *buffer);
int send_chunks(const int sockfd, const CLIENT from, const CLIENT to,
                const readable *stream);
int recv_chunks(const int sockfd, writeable buffer, CLIENT *from);
int send_text(const int sockfd, const char *message, CLIENT from, CLIENT to);
int _recv_text(const int sockfd, const char *message, CLIENT from, CLIENT to);
MSG_TYPE revive_any(const info from, void *buffer);
#endif
