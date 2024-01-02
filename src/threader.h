#ifndef _THREADER_
#define _THREADER_

#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdint.h>

#define log(fd, x, ...) fprintf(fd, x, ##__VA_ARGS__);

typedef struct {
        uint8_t max;
        uint8_t count;
        pthread_attr_t *attrs;
        pthread_t *threads;
        /*
            create should return it's index on threads on sucess and -1 on faild
        */
        int (*create)(void *(*callback)(void *), void *args);
        void *(*join)(int index);
        int (*cancle)(int index);
        int (*stop)(int index);
        void (*clean)();
} thread_obj;

void thread_obj_setup(uint8_t max, pthread_attr_t *attrs);
int create(void *(*callback)(void *), void *args);
void *join(int index);
int cancle(int index);
int stop(int index);

static thread_obj THREAD_OBJECT = {
    .max = 0xff,
    .count = 0,
    .threads = NULL,
    .create = &create,
    .join = &join,
    .cancle = &cancle,
    .stop = &stop,
};

#endif
