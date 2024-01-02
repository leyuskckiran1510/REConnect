#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "./../def.h"
#include "./../message.h"
#include "./../threader.h"

#define PORT 12345
#define MAX_BUFFER_SIZE 1024
#define TIMEOUT_SEC 1
#define TIMEOUT_MILISEC 0


typedef struct{
    struct sockaddr_in addres;
    uint8_t occupied;
}Clients;

Clients ALL_CLIENTS[100] = {0};

void display_info(info *inf) {
    if (inf != NULL) {
        print("SOCK_DESCRIPTOR [%d],SIGNAL_STATE [%d] ", inf->sockfd,
              *inf->signal);
    } else {
        print("NULL");
    }
}

// TODO; improve this to reduce hash collision
int generate_index(const struct sockaddr_in a) {
    uint32_t ip = a.sin_addr.s_addr << 11;
    uint32_t port = a.sin_port << 7;
    return (ip ^ port) % 100;
}

int is_empty(const struct sockaddr_in a) {
    //at least one bit will match if their are any data
    return !(a.sin_port & a.sin_addr.s_addr);
}

void *fn_input_handler(void *_args) {

    info *inf = (info *)_args;
    char buffer[50];
   
    while (!*inf->signal) {
        fgets(buffer, 50, stdin);
        if (strncmp(buffer, "stop", 4) == 0) {
            MESSAGE m = {
                .header.type = E_TEXT_MESSGAE,
                .header.length = 5,
                .data.text = "stop",
            };
            gettimeofday(&m.data.timestamp,NULL);
            CLIENT c={0};
            m.data.from = c;
            for (int i = 0; i < 100; i++) {
                if (!ALL_CLIENTS[i].occupied) {
                    continue;
                }
                int send_status = sendto(inf->sockfd, &m,sizeof(m), 0,
                                         (struct sockaddr *)&ALL_CLIENTS[i].addres,
                                         sizeof(struct sockaddr));
                if (send_status == -1) {
                    error(strerror(errno));
                }
            }
            *inf->signal = 1;

        } else {
            memset(buffer, 0, strlen(buffer));
        }
    }
    return (void *)inf;
}

void *fn_reciver(void *_args) {
    info *inf = (info *)_args;
    size_t client_len = sizeof(struct sockaddr_in);
    char buffer[MAX_BUFFER_SIZE];
    while (!*inf->signal) {
        if (!has_data(inf->sockfd)) {
            continue;
        }
        int recv_len = recvfrom(inf->sockfd, buffer, MAX_BUFFER_SIZE, 0,
                                (struct sockaddr *)&inf->sockaddr,
                                (socklen_t *)&client_len);

        if (recv_len == -1) {
            error("Error receiving data");
        }

        MESSAGE *m = (MESSAGE *)buffer;
        int index = generate_index(inf->sockaddr);

        if (ALL_CLIENTS[index].occupied) {
            print("Last Port %x ", ALL_CLIENTS[index].addres.sin_port);
        } else {
            print("New Connection Detected: %.*s\n"
                  " Username [%s] \n",
                  recv_len, buffer, m->data.from.username);
            ALL_CLIENTS[index].addres = inf->sockaddr;
            ALL_CLIENTS[index].occupied = 1;
        }

        if (m->header.type == E_HELLO) {
            print("CLIETN SAID HELLO TO ME\nRESPONDING BACK");
            CLIENT c={0};
            send_hello(*(info *)_args, c);
            continue;
        }
        if (m->header.type == E_TEXT_MESSGAE) {
            for (int i = 0; i < 100; i++) {
                if (!ALL_CLIENTS[i].occupied || index == i) {
                    continue;
                }
                int send_status =
                    sendto(inf->sockfd, buffer, recv_len, 0,
                           (struct sockaddr *)&ALL_CLIENTS[i].addres, client_len);
                if (send_status == -1) {
                    error(strerror(errno));
                }
            }
        }
    }
    return (void *)inf;
}

int main() {

    thread_obj_setup(2, NULL);

    int sockfd;
    int signal = 0;
    struct sockaddr_in server_addr;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        error("Error creating socket");
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
        -1) {
        error("Error binding socket");
    }

    info t1 = {.sockfd = sockfd, .sockaddr = server_addr, .signal = &signal};
    int send_index = THREAD_OBJECT.create(fn_input_handler, (void *)&t1);
    if (send_index < 0) {
        error("[SERVER] Could Not Initialize Thread fro Sender");
    } else {
        print("Sucessfully Created Sender");
    }

    info t2 = {.sockfd = sockfd, .sockaddr = server_addr, .signal = &signal};
    int recive_index = THREAD_OBJECT.create(fn_reciver, (void *)&t2);
    if (recive_index < 0) {
        error("[SERVER] Could Not Initialize Thread fro Sender");
    } else {
        print("Sucessfully Created Reciver");
    }

    printf("Running on port localhost:%d...\n", PORT);

    void *result;
    result = THREAD_OBJECT.join(send_index);
    display_info(result);
    result = THREAD_OBJECT.join(recive_index);
    display_info(result);

    close(sockfd);

    return 0;
}
