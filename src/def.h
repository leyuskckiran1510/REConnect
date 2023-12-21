#ifndef __DEFINATION__
#define __DEFINATION__

#include <netinet/in.h>
#include <stdint.h>

#define MAX_USER_NAME 250 

#define print_client(C)                                                        \
  printf("(Username):-[%s] (Ip):-[%d:%d:%d:%d] (Port):-%u\n", c.username,      \
         c.address.ip1, c.address.ip2, c.address.ip3, c.address.ip4, c.address.port);
#define print(x, ...) printf(x "\n", ##__VA_ARGS__)

typedef union {
    /* the total size of this is sizeof(sockaddr_in) */
    struct sockaddr_in sock_addr_in;
    /*
        Spreading sockaddr_in componets for
        easier access
    */
    struct {
        sa_family_t family;
        in_port_t port;
        union {
            uint32_t ip;
            /*
                Spreading each byte in ip
                to individual elements for
                easier access (mainly for printing and debugging)
            */
            struct {
                uint8_t ip1;
                uint8_t ip2;
                uint8_t ip3;
                uint8_t ip4;
            };
        };
    };
} ADDRESS_IN;

typedef struct {
    uint32_t online;
    char *last_message;
    ADDRESS_IN address;
    char username[MAX_USER_NAME];
} CLIENT;

typedef struct {
    uint32_t ip;
    CLIENT *clients;
    ADDRESS_IN address;
    int client_count;
} SERVER;

#endif