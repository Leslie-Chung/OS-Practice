#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>

#define WORKER_NUMBER 4
#define CAPACITY 8


typedef struct Task {
    int is_end;
    char path[128];
    char string[128];
}task;

task *buffer[CAPACITY];
int in;
int out;

int buffer_is_empty()
{
    return in == out;
}

int buffer_is_full()
{
    return (in + 1) % CAPACITY == out;
}

task* get_item()
{
    task *item;

    item = buffer[out];
    out = (out + 1) % CAPACITY;
    return item;
}

void put_item(task *item)
{
    buffer[in] = item;
    in = (in + 1) % CAPACITY;
}

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

sema_t mutex_sema;
sema_t empty_buffer_sema;
sema_t full_buffer_sema;

void find_file(char *path, char *target)
{
    // puts(path);
    FILE *file = fopen(path, "r");
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, target))
            printf("%s: %s", path, line);
    }
    fclose(file);
}

void *worker_entry(void *arg)
{
    while (1) {
        sema_wait(&full_buffer_sema);
        sema_wait(&mutex_sema);

        task *item = get_item();
        sema_signal(&mutex_sema);
        sema_signal(&empty_buffer_sema);
        
        if(item->is_end)
            break;
        find_file(item->path, item->string);
        free(item);
    }
    return NULL;
}

void *produce(char *path, char *target, int first)
{
    DIR *dir = opendir(path);
    struct dirent *entry;
    char pathstr[256];
    sprintf(pathstr, "%s/", path);

    while (entry = readdir(dir)) {
        if (strcmp(entry->d_name, ".") == 0)
            continue;
        else if (strcmp(entry->d_name, "..") == 0)
            continue;
        else if (entry->d_type == DT_DIR) {
            // printf("dir %s\n", entry->d_name);
            char dirstr[512];
            sprintf(dirstr, "%s%s", pathstr, entry->d_name);
            produce(dirstr, target, 0);
        }           
        else if (entry->d_type == DT_REG) {
            // printf("file %s\n", entry->d_name);
            char dirstr[512];
            sprintf(dirstr, "%s%s", pathstr, entry->d_name);
            
            sema_wait(&empty_buffer_sema);
            sema_wait(&mutex_sema);
            task *item = (task*) malloc(sizeof(task));
            strcpy(item->path, dirstr);
            strcpy(item->string, target);
            put_item(item);

            sema_signal(&mutex_sema);
            sema_signal(&full_buffer_sema);
        }
    }
    if(first){
        int i;
        for (i = 0; i < WORKER_NUMBER; i++) {
            sema_wait(&empty_buffer_sema);
            sema_wait(&mutex_sema);
            task *item = (task*) malloc(sizeof(task));
            item->is_end = 1;
            put_item(item);
            sema_signal(&mutex_sema);
            sema_signal(&full_buffer_sema);
        }
    }

    return NULL;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        puts("Usage: pfind file string");
        return 0;
    }
    
    char *path = argv[1];
    char *string = argv[2];
    struct stat info;
    stat(path, &info);
    if (!S_ISDIR(info.st_mode)){
        find_file(path, string);
        return 0;
    }
    
    sema_init(&mutex_sema, 1);
    sema_init(&empty_buffer_sema, CAPACITY - 1);
    sema_init(&full_buffer_sema, 0);
    
    pthread_t workers[WORKER_NUMBER];
    int i;

    for (i = 0; i < WORKER_NUMBER; i++) {
        pthread_create(&workers[i], NULL, worker_entry, NULL);
    }

    produce(path, string, 1);
    
    
    for (i = 0; i < WORKER_NUMBER; i++) {
        pthread_join(workers[i], NULL);
    }
        
    return 0;
}
