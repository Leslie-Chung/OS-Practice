#include<stdio.h>
#include<pthread.h>
#include<stdlib.h>
#include<time.h>
typedef struct param{
    int start, end;
}param;

#define ARR_LEN 1024
int n;
int nums[ARR_LEN];

void *sort(void *arg){
    param *p = (param*) arg;
    for (int i = p->start; i < p->end; i++) {
        int min = i;
        for (int j = i; j < p->end; j++)
            if (nums[j] < nums[min]) min = j;
        int temp = nums[i];
        nums[i] = nums[min];
        nums[min] = temp;
    }
    return NULL;
}

int main(int argc, char *argv[]){
    if (argc < 2) {
        puts("too few arguments");
        return 0;
    }

    sscanf(argv[1], "%d", &n);

    if (n < 0 || n > ARR_LEN) {
        puts("the array's length is invalid");
        return 0;
    }
    
    int i;
    srand((unsigned int)time(NULL));
    for(i = 0;i < n;i++)
        nums[i] = rand() % 100;
    
    for(i = 0;i < n;i++)
        printf("%d ", nums[i]);
    puts("");

    pthread_t workers[2];
    param params[2];

    for(i = 0;i < 2;i++){
        param *param;
        param = &params[i];
        param->start = i * n / 2;
        param->end = (i + 1) * n / 2;
        pthread_create(&workers[i], NULL, &sort, param);
    }
    
    float sum = 0;
    for(i = 0;i < 2;i++)
        pthread_join(workers[i], NULL);
    
    int result[ARR_LEN];
    int p = 0, q = n / 2, r = 0;
    int right = n / 2;
    while (p < n / 2 || q < n) {
        if (p >= right && q < n) result[r++] = nums[q++];
        if (p < right && q >= n) result[r++] = nums[p++];
        if (p < right && q < n)
            result[r++] = nums[p] < nums[q] ? nums[p++] : nums[q++];
    }

    for(i = 0;i < n;i++)
        printf("%d ", result[i]);
    puts("");
}
