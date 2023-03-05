#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include<stdlib.h>

typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} sema_t;

sema_t mutex_sema_ping, mutex_sema_pong;


void sema_init(sema_t *sema, int value)
{
    sema->value = value;
    pthread_mutex_init(&sema->mutex, NULL);
    pthread_cond_init(&sema->cond, NULL);
}

void sema_wait(sema_t *sema)
{
    pthread_mutex_lock(&sema->mutex);
    while (sema->value <= 0)
        pthread_cond_wait(&sema->cond, &sema->mutex);
    sema->value--;
    pthread_mutex_unlock(&sema->mutex);
}

void sema_signal(sema_t *sema)
{
    pthread_mutex_lock(&sema->mutex);
    ++sema->value;
    pthread_cond_signal(&sema->cond);
    pthread_mutex_unlock(&sema->mutex);
}

int capacity = 8;

void *ping(void *arg)
{
    int i;
    for(i = 0; i < capacity; i++){
        sema_wait(&mutex_sema_ping);
        puts("ping");        
        sema_signal(&mutex_sema_pong);
    }
    return NULL;
}

void *pong()
{
    int i;
    for(i = 0; i < capacity; i++){
        sema_wait(&mutex_sema_pong);
        puts("pong");        
        sema_signal(&mutex_sema_ping);
    }
    return NULL;
}


int main(){
    pthread_t ping_tid;
    
    sema_init(&mutex_sema_ping, 1);
    sema_init(&mutex_sema_pong, 0);
    
    pthread_create(&ping_tid, NULL, ping, NULL);
    pong();

    pthread_join(ping_tid, NULL);

}
