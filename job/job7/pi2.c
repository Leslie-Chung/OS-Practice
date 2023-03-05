#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
typedef struct param{
    int start, end;
}param;

typedef struct result{
    float sum;
}result;

#define NR_CPU 100

int n;
int thread_num;

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
    if (argc < 3) {
        puts("too few arguments");
        return 0;
    }

    sscanf(argv[1], "%d", &n);
    sscanf(argv[2], "%d", &thread_num);

    if (n <= 0) {
        puts("the depth is invalid");
        return 0;
    }

    if (n == 1) {
        puts("1");
        return 0;
    }

    if(thread_num <= 1 || thread_num > n || thread_num > 100){
        puts("the thread_num is invalid");
        return 0;
    }

    pthread_t workers[NR_CPU];
    param params[NR_CPU];
    result *res[NR_CPU];

    int i;
    for(i = 0;i < thread_num;i++){
        param *param;
        param = &params[i];
        param->start = i * n / thread_num + 1;
        param->end = (i + 1) * n / thread_num;
        pthread_create(&workers[i], NULL, &thread_func, param);
    }
    
    float sum = 0;
    for(i = 0;i < thread_num;i++){
        result *res;
        pthread_join(workers[i], (void **)&res);
        sum += res->sum;
        free(res);
    }

    sum *= 4;
    printf("%f\n", sum);

}
