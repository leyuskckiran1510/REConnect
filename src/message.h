#ifndef __MESSAGE_DESCRIPTION__
#define __MESSAGE_DESCRIPTION__

#include "./def.h"
#include <stdint.h>
#include <time.h>

#define CHECKSUM_SIZE 100
typedef enum {
  E_CLIENT_HELLO = 0,
  E_SERVER_HELLO,
  E_HEART_BEAT,
  E_TEXT_MESSGAE,
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
    union {
        char text[100];    // for TEXT_MESSAGE
        char content[100]; // for AV_MESSGAES(Audio,Video and Both)
    };
    struct timeval timestamp;
} MSG_DATA;

typedef struct {
    MSG_HEADER header;
    MSG_DATA data;
} MESSAGE;

typedef struct{
    MSG_HEADER header;
    union{
        uint32_t ping;
        uint32_t pong;
    };

} M_HEARTBEAT;


typedef struct{
    MSG_HEADER header;
    union{
        uint32_t from_client;
        uint32_t from_server;
    };
} M_HELLO;

#endif