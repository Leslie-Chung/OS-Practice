#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>

int capacity = 8;
pthread_mutex_t mutex;
pthread_cond_t wait_ping_buffer;
pthread_cond_t wait_pong_buffer;

void *ping(void *arg)
{
    int i;
    for(i = 0; i < capacity; i++){
        pthread_mutex_lock(&mutex);
        puts("ping");        

        pthread_cond_signal(&wait_pong_buffer);
        if(i != capacity - 1) 
            pthread_cond_wait(&wait_ping_buffer, &mutex);

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void *pong()
{
    int i;
    for(i = 0; i < capacity ; i++){
        pthread_mutex_lock(&mutex);
        puts("pong");

        pthread_cond_signal(&wait_ping_buffer);
        if(i != capacity - 1) // 最后一次不需要再阻塞了
            pthread_cond_wait(&wait_pong_buffer, &mutex);

        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}


int main(){
    pthread_t ping_tid;
    pthread_create(&ping_tid, NULL, ping, NULL);
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&wait_pong_buffer, &mutex);
    pthread_mutex_unlock(&mutex);
    pong();

    pthread_join(ping_tid, NULL);
}
