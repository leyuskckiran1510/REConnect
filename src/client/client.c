#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <time.h>

#include <pthread.h>

#include "./../def.h"
#include "./../message.h"
#include "./../threader.h"
#include <sys/time.h>

#define MAX_BUFFER_SIZE 1000024
#define TIMEOUT_SEC 1
#define TIMEOUT_MILISEC 0
#define __local__
#ifdef __local__
    #define SERVER "127.0.0.1"
    #define PORT 12345
#endif

void display_info(info *inf) {
    if (inf != NULL) {
        print("SOCK_DESCRIPTOR [%d],SIGNAL_STATE [%d] ", inf->sockfd,
              *inf->signal);
    } else {
        print("NULL");
    }
}

void *fn_sender(void *_args) {
    info *inf = (info *)_args;
    char buffer[MAX_BUFFER_SIZE];
    while (!*inf->signal) {
        // printf("\x1b[2J\x1b[10000;0H | Enter data to send (or exit to
        // quit):-");
        printf(":~\x1b[4m ");
        if (!has_data(0)) {
            continue;
        }

        fgets(buffer, MAX_BUFFER_SIZE, stdin);

        buffer[strlen(buffer) - 1] = 0;

        if (strncmp(buffer, "stop", 4) == 0) {
            *inf->signal = 1;
            break;
        }
        if (strlen(buffer) > 0) {
            MSG_TYPE response = send_text(inf->sockfd, buffer,inf->self,inf->self);
            if(response==E_ERROR){
                error("Error Sending Text Message");
            }
        }
    }
    return (void *)inf;
}

void *fn_reciver(void *_args) {
    info *inf = (info *)_args;
    info new_info = {.sockfd=inf->sockfd};
    char _buffer[MAX_BUFFER_SIZE];
    while (!*inf->signal) {
        // socklen_t serve_add_len = sizeof(inf->sockaddr);
        if (!has_data(inf->sockfd)) {
            continue;
        }
        // memset(&buffer, 0, MAX_BUFFER_SIZE);
        // int recv_len =
        //     recvfrom(inf->sockfd, buffer, MAX_BUFFER_SIZE, 0,
        //              (struct sockaddr *)&inf->sockaddr, &serve_add_len);
        MSG_TYPE recv_len =  revive_any(new_info,&_buffer);
        MESSAGE buffer = *(MESSAGE*)_buffer;
        if (recv_len == E_ERROR) {
            error("Error Receving Response from server");
        }
        printf("[Buffer] %s\n",_buffer);
        if (strncmp(buffer.data.text, "stop", 4) == 0) {
            fprintf(stderr, "Server Stopped THe Connection");
            *inf->signal = 1;
        }

        if (strlen(buffer.data.text) > 1) {
            print("[%s]: %s", buffer.data.from.username,buffer.data.text);
        }
    }
    return (void *)inf;
}

int main() {
    thread_obj_setup(2, NULL);

    int signal = 0;
    int sockfd;
    struct sockaddr_in sockaddr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        error("Error creating socket");
    }

    CLIENT c = {.online = 1};
    printf("Please Enter Your UserName:- ");
    scanf("%[^\n]+\n", c.username);

    memset(&sockaddr, 0, sizeof(sockaddr));

#ifndef __local__
    char SERVER[3 * 4 + 4]; // 255.255.255.255
    uint PORT;
    printf("Server Address");
    scanf("\n%[^\n]+\n", SERVER);
    printf("Server Port ");
    scanf("%u", &PORT);
#endif
    print("Making Conenction to[%s:%u]", SERVER, PORT);

    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = inet_addr(SERVER);
    sockaddr.sin_port = htons(PORT);
    c.address.sock_addr_in = sockaddr;

    print_client(c);

    info t1 = {
        .sockfd = sockfd, .sockaddr = sockaddr, .signal = &signal, .self = c
    };

    if (send_hello(t1, c) == -1) {
        error("Error Sending/Receving Hello");
        return 1;
    }

    int send_index = THREAD_OBJECT.create(fn_sender, (void *)&t1);
    int recive_index = THREAD_OBJECT.create(fn_reciver, (void *)&t1);

    if (send_index < 0) {
        error("[SERVER] Could Not Initialize Thread fro Sender");
    } else {
        print("Sucessfully Created Sender");
    }
    if (recive_index < 0) {
        error("[SERVER] Could Not Initialize Thread fro Sender");
    } else {
        print("Sucessfully Created Reciver");
    }

    void *result;
    result = THREAD_OBJECT.join(send_index);
    display_info(result);
    result = THREAD_OBJECT.join(recive_index);
    display_info(result);

    close(sockfd);

    return 0;
}
