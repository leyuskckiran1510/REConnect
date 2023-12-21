#include "threader.h"
#include <arpa/inet.h>
#include <bits/pthreadtypes.h>
#include <stdio.h>
#include <stdlib.h>

void clean(){
    if(THREAD_OBJECT.threads!=NULL){
        free(THREAD_OBJECT.threads);
    }
    THREAD_OBJECT.threads=NULL;
}

void thread_obj_setup(uint8_t max,pthread_attr_t *attrs){
    THREAD_OBJECT.attrs = attrs;
    THREAD_OBJECT.max = max;
    THREAD_OBJECT.clean = &clean;
    THREAD_OBJECT.threads = calloc(max,sizeof(pthread_t));
    atexit(clean);
}
int create(void *(*callback)(void *), void *args){
    pthread_t new;
    int created = pthread_create(&new, THREAD_OBJECT.attrs,callback,args);
    if (created != 0) {
        return -1;
    }
    uint8_t index = THREAD_OBJECT.count++;
    THREAD_OBJECT.threads[index] = new;
    return index;

}

void *join(int index){
    void *result;
    if(index<0 || index>THREAD_OBJECT.max){
        return NULL;
    }
    if (pthread_join(THREAD_OBJECT.threads[index], &result) != 0) {
        log(stderr, "Error Joining Thread at index [%d] with Id [%lu]" ,index,THREAD_OBJECT.threads[index]);
        return NULL;
    }else{
        return result;
    }


}
int cancle(int index){
    (void)index;
    return 0;
}
int stop(int index){
    (void)index;
    return 0;
}
