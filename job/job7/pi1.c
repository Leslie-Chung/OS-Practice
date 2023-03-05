#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
typedef struct param{
    int start, end;
}param;

typedef struct result{
    float sum;
}result;

#define NR_CPU 1
int n;

void *calculate(void *arg){
    param *p = (param*) arg;
    int i;
    result *res = (result *)malloc(sizeof(result));
    res->sum = 0;
    for(i = p->start;i <= p->end;i++){
        res->sum += i % 2 == 0 ? -1.0 / (i * 2 - 1) : 1.0 / (i * 2 - 1);
    }
    return (void *) res;
}

void *thread_func(void *arg) {
    return calculate(arg);
}


int main(int argc, char *argv[]){
    if (argc < 2) {
        puts("too few arguments");
        return 0;
    }

    sscanf(argv[1], "%d", &n);
    if (n <= 0) {
        puts("the depth is too small");
        return 0;
    }

    if (n == 1) {
        puts("1");
        return 0;
    }

    pthread_t workers[NR_CPU];
    param params[NR_CPU + 1];
    result *res[NR_CPU + 1];
    params[1].start = n / 2 + 1;
    params[1].end = n;
    pthread_create(&workers[0], NULL, &thread_func, &params[1]);

    params[0].start = 1;
    params[0].end = n / 2;
    res[0] = (result *) calculate((void *) &params[0]);
    
    pthread_join(workers[0], (void **) (res + 1));
    float sum = res[0]->sum;
    sum += res[1]->sum;
    sum *= 4;
    printf("%f\n", sum);
    free(res[0]);
    free(res[1]);
}
