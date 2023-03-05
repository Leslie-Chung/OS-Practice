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

pthread_mutex_t mutex_pc, mutex_cc;
pthread_cond_t wait_empty_buffer_pc, wait_empty_buffer_cc;
pthread_cond_t wait_full_buffer_pc, wait_full_buffer_cc;

void *consume(void *arg)
{
    int i;
    int item;
    int item_count = container_cc.capacity * 2;

    for (i = 0; i < item_count; i++) { 
        pthread_mutex_lock(&mutex_cc);
        while (buffer_is_empty(&container_cc))
            pthread_cond_wait(&wait_full_buffer_cc, &mutex_cc);

        item = get_item(&container_cc);
        printf("        %c\n", item); 

        pthread_cond_signal(&wait_empty_buffer_cc);
        pthread_mutex_unlock(&mutex_cc);
    }

    return NULL;
}

void *produce()
{
    int i;
    int item;
    int item_count = container_pc.capacity * 2;

    for (i = 0; i < item_count; i++) { 
        pthread_mutex_lock(&mutex_pc);
        while (buffer_is_full(&container_pc))
            pthread_cond_wait(&wait_empty_buffer_pc, &mutex_pc);

        item = i + 'a';
        put_item(&container_pc, item);
        printf("%c\n", item);

        pthread_cond_signal(&wait_full_buffer_pc);
        pthread_mutex_unlock(&mutex_pc);
    }

    return NULL;
}

void *compute()
{
    int i;
    int item;
    int item_count = container_pc.capacity * 2;

    for (i = 0; i < item_count; i++) {
        pthread_mutex_lock(&mutex_pc);
        while (buffer_is_empty(&container_pc))
            pthread_cond_wait(&wait_full_buffer_pc, &mutex_pc);

        item = get_item(&container_pc);
        printf("    %c:%c\n", item, item - 'a' + 'A');

        pthread_cond_signal(&wait_empty_buffer_pc);
        pthread_mutex_unlock(&mutex_pc);

        pthread_mutex_lock(&mutex_cc);
        while (buffer_is_full(&container_cc))
            pthread_cond_wait(&wait_empty_buffer_cc, &mutex_cc);

        item = item - 'a' + 'A';
        put_item(&container_cc, item);
        
        pthread_cond_signal(&wait_full_buffer_cc);
        pthread_mutex_unlock(&mutex_cc);
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

    pthread_create(&consumer_tid, NULL, consume, NULL);
    pthread_create(&compute_tid, NULL, compute, NULL);

    produce();
    pthread_join(consumer_tid, NULL);
    pthread_join(compute_tid, NULL);
    return 0;
}
