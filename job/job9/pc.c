#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#define CAPACITY 4

typedef struct {
    int capacity;
    int *buffer;
    int in;
    int out;
} container_t;

void container_init(container_t *container, int capacity)
{
    container->capacity = capacity;
    container->buffer = (int *) malloc(sizeof(int) * capacity);
    container->in = container->out = 0;
}

int buffer_is_empty(container_t *container)
{
    return container->in == container->out;
}

int buffer_is_full(container_t *container)
{
    return (container->in + 1) % container->capacity == container->out;
}

int get_item(container_t *container)
{
    int item;

    item = container->buffer[container->out];
    container->out = (container->out + 1) % container->capacity;
    return item;
}

void put_item(container_t *container, int item)
{
    container->buffer[container->in] = item;
    container->in = (container->in + 1) % container->capacity;
}

container_t container_pc, container_cc;

typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} sema_t;

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

sema_t mutex_sema_pc, mutex_sema_cc;
sema_t empty_buffer_sema_pc, empty_buffer_sema_cc;
sema_t full_buffer_sema_pc, full_buffer_sema_cc;

void *consume(void *arg)
{
    int i;
    int item;
    int item_count = container_cc.capacity * 2;

    for (i = 0; i < item_count; i++) { 
        sema_wait(&full_buffer_sema_cc);
        sema_wait(&mutex_sema_cc);

        item = get_item(&container_cc);
        printf("        %c\n", item); 

        sema_signal(&mutex_sema_cc);
        sema_signal(&empty_buffer_sema_cc);
    }

    return NULL;
}

void *produce()
{
    int i;
    int item;
    int item_count = container_pc.capacity * 2;

    for (i = 0; i < item_count; i++) { 
        sema_wait(&empty_buffer_sema_pc);
        sema_wait(&mutex_sema_pc);

        item = i + 'a';
        put_item(&container_pc, item);
        printf("%c\n", item); 

        sema_signal(&mutex_sema_pc);
        sema_signal(&full_buffer_sema_pc);
    }

    return NULL;
}

void *compute()
{
    int i;
    int item;
    int item_count = container_pc.capacity * 2;

    for (i = 0; i < item_count; i++) {
        sema_wait(&full_buffer_sema_pc);
        sema_wait(&mutex_sema_pc);

        item = get_item(&container_pc);
        printf("    %c:%c\n", item, item - 'a' + 'A');

        sema_signal(&mutex_sema_pc);
        sema_signal(&empty_buffer_sema_pc);

        sema_wait(&empty_buffer_sema_cc);
        sema_wait(&mutex_sema_cc);

        item = item - 'a' + 'A';
        put_item(&container_cc, item);

        sema_signal(&mutex_sema_cc);
        sema_signal(&full_buffer_sema_cc);

    }

    return NULL;
}

int main()
{ 
    pthread_t consumer_tid, compute_tid;
    int capacity_pc = 4;
    int capacity_cc = 4;

    container_init(&container_pc, capacity_pc);
    container_init(&container_cc, capacity_cc);

    sema_init(&mutex_sema_pc, 1);
    sema_init(&mutex_sema_cc, 1);

    sema_init(&empty_buffer_sema_pc, capacity_pc - 1);
    sema_init(&empty_buffer_sema_cc, capacity_cc - 1);
    
    sema_init(&full_buffer_sema_pc, 0);
    sema_init(&full_buffer_sema_cc, 0);

    pthread_create(&consumer_tid, NULL, consume, NULL);
    pthread_create(&compute_tid, NULL, compute, NULL);

    produce();
    pthread_join(consumer_tid, NULL);
    pthread_join(compute_tid, NULL);
    return 0;
}
