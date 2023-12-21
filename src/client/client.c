#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <time.h>

#include <pthread.h>

#include "./../def.h"
#include "./../message.h"
#include "./../threader.h"
#include <sys/time.h>

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define MAX_BUFFER_SIZE 1000024
#define TIMEOUT_SEC 1
#define TIMEOUT_MILISEC 0

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

typedef struct {
        int sockfd;
        struct sockaddr_in server_addr;
        int *signal;
} info;

void display_info(info *inf) {
    if (inf != NULL) {
        print("SOCK_DESCRIPTOR [%d],SIGNAL_STATE [%d] ", inf->sockfd,
              *inf->signal);
    } else {
        print("NULL");
    }
}

int has_data(int sock) {
    int retval;
    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(sock, &rfds);
    tv.tv_sec = TIMEOUT_SEC;
    tv.tv_usec = TIMEOUT_MILISEC;
    retval = select(sock + 1, &rfds, NULL, NULL, &tv);
    return retval;
}

void *fn_sender(void *_args) {
    info *inf = (info *)_args;
    char buffer[MAX_BUFFER_SIZE];
    while (!*inf->signal) {
        printf("\x1b[2J\x1b[10000;0H | Enter data to send (or exit to quit):-");
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
            if (sendto(inf->sockfd, buffer, strlen(buffer), 0,
                       (struct sockaddr *)&inf->server_addr,
                       sizeof(inf->server_addr)) == -1) {
                error("Error sending data");
            }
        }
    }
    return (void *)inf;
}

void *fn_reciver(void *_args) {
    info *inf = (info *)_args;
    char buffer[MAX_BUFFER_SIZE];

    while (!*inf->signal) {
        socklen_t serve_add_len = sizeof(inf->server_addr);
        if (!has_data(inf->sockfd)) {
            printf("\x1b[9;10000H\r \t\t\t\t[ %s ]\x1b[10000;43H", buffer);
            fflush(stdout);
            continue;
        }
        memset(buffer, 0, MAX_BUFFER_SIZE);
        int recv_len =
            recvfrom(inf->sockfd, buffer, MAX_BUFFER_SIZE, 0,
                     (struct sockaddr *)&inf->server_addr, &serve_add_len);
        if (recv_len == -1) {
            error("Error Receving Response from server");
        }
        if (strncmp(buffer, "stop", 4) == 0) {
            fprintf(stderr, "Server Stopped THe Connection");
            *inf->signal = 1;
        }
        printf("\x1b[9;10000H\r \t\t\t\t[ %s ]\x1b[10000;43H", buffer);
        fflush(stdout);
    }
    return (void *)inf;
}

int send_hello(const info a,const CLIENT c) {
    MESSAGE m = {.header = {.type = E_CLIENT_HELLO, .sequence = 0},
                 .data = {.to = c, .from = c}};
    clock_gettime(0, (struct timespec *)&m.data.timestamp);
     
    memcpy(m.data.text, "HELLO FROM CLIENT",17);
    int _s = sendto(a.sockfd, (void *)&m, sizeof(m), 0,
           (struct sockaddr *)&a.server_addr, sizeof(struct sockaddr_in));
    if(_s==-1){
        error("Error Sending Client Hello");
        return -1;
    }
    size_t read_length=0;
    if(!has_data(a.sockfd)){
        error("Didnot Recvived The Server Hello");
        return -1;
    }
    
    _s = recvfrom(a.sockfd,(void *)&m,sizeof(MESSAGE),0,
                  (struct sockaddr*)&a.server_addr,(socklen_t*)&read_length);
    if(_s==-1){
        error("Couldnot Understand Server Hello");
        return -1;
    }
    return 0;
}

int main() {
    thread_obj_setup(2, NULL);

    int signal;
    int sockfd;
    struct sockaddr_in server_addr;
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        error("Error creating socket");
    }

    CLIENT c = {.address = {server_addr}, .online = 1};
    printf("Please Enter Your UserName:- ");
    scanf("%[^\n]+", c.username);
    print_client(c);

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(PORT);

    info t1 = {.sockfd = sockfd, .server_addr = server_addr, .signal = &signal};
    
    if(send_hello(t1,c)==-1){
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
