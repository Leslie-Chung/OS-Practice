[TOC]

# 南京航空航天大学《操作系统实践》报告

* 姓名：马睿

* 班级：1619304

* 学号：161930131




# job6/sh3.c

## 题目要求

实现shell程序，要求支持基本命令、重定向命令、管道命令、后台命令



## 解决思路

1. 利用编译原理中递归下降分析器的方法，对命令进行语法分析
2. 建立一种数据结构，可以表示不同种类命令的组合关系以及先后执行关系，来辅助完成递归下降分析



## 关键代码

### 结构体tree

以树的方式存储命令的语法，即通过语法树来表示不同种类命令的组合关系以及先后执行关系。

树的每个节点都含有如下的参数：

```c
enum {
    TREE_BACK,
    TREE_PIPE,
    TREE_REDIRICT,
    TREE_BASIC,
    TREE_TOKEN,
};

typedef struct {
    int type;
    char *token;           
    vector_t child_vector;
} tree_t;
```



**type**表示这棵子树（命令）的类型，分别有：

- TREE_BACK（&），说明该命令是后台命令
- TREE_PIPE（|），说明该命令是管道命令，肯定会有两个孩子节点
- TREE_REDIRICT（>、>>、<），说明该命令是重定向命令
- TREE_BASIC，说明该命令是基本命令
- TREE_TOKEN，说明该子树是叶子节点，无法将其拆分，包含了命令的各个参数



**token**用来存储命令的参数

- 若节点是非叶子节点，token存储的是该命令类型的字符串，如类型是TREE_BACK，则token == back；类型是TREE_REDIRICT，则token == redirect
- 若节点是叶子节点，token存储的是该基本命令的各个参数



**child_vector**则用来存储当前节点的孩子节点

### main.c中的关键函数

#### execute_line

用来将用户输入的整个命令line，解析为语法树并从树的根节点遍历执行（递归下降分析器），如果需要输出树的结构，则令verbose非0即可。

执行完后销毁掉开辟的空间。

```c
void execute_line(char *line)
{
    tree_t *tree;
    lex_init(line);
    tree = parse_tree();
    if (verbose)
        tree_dump(tree, 0);
    if (tree != NULL) 
        tree_execute_wrapper(tree);
    lex_destroy();
}
```

#### read_line

将长度为size的字符串line拆分为多个单词，即将命令拆分为一个个单词

```c
void read_line(char *line, int size)
{
    int count;

    count = read(0, line, size);
    if (count == 0)
        exit(EXIT_SUCCESS);
    assert(count > 0);
    if ((count > 0) && (line[count - 1] == '\n'))
        line[count - 1] = 0;
    else
        line[count] = 0;
}
```

#### read_and_execute

用来读取命令，并对命令进行处理，即整个程序的入口

```c
void read_and_execute()
{
    char line[128];

    write(1, "# ", 2);
    read_line(line, sizeof(line));
    execute_line(line);
}
```

#### test

用来测试数据

```C
void test()
{
    execute_line("cat /etc/passwd | sort | grep root >log");
}
```



### exec.c 中的关键函数

#### tree_execute_redirect

用来执行重定向命令。

对于重定向类型的节点

- 命令的主体部分是第一个孩子节点
- 命令的符号（> / >> / <）是第二个孩子节点
- 命令的重定向文件是第三个孩子节点
- 将三个信息提取出来之后，根据命令的符号进行相应的重定向并执行命令的主体部分即可。

```c
void tree_execute_redirect(tree *root)
{
    tree *body = root->child_vector[0];
    tree *operator = root->child_vector[1];
    tree *file = root->child_vector[2];
    int fd;
    int redirect_fd;

    if(strcmp(operator->token, "<") == 0){
        fd = open(file->token, O_RDONLY);
        redirect_fd = 0;
    }

    if(strcmp(operator->token, ">") == 0){
        fd = open(file->token, O_WRONLY | O_CREAT | O_TRUNC, 00777);
        redirect_fd = 1;
    }

    if(strcmp(operator->token, ">>") == 0){
        fd = open(file->token,O_WRONLY | O_APPEND, 00777);
        redirect_fd = 1;
    }
    
    dup2(fd, redirect_fd);
    tree_execute(body);
}
```



#### tree_execute_basic

用来执行基本命令

其孩子节点必定全部是叶子节点，即基本命令的各个参数，将其提取出来并使用execvp执行即可。

```C
void tree_execute_basic(tree *root)
{
    int argc = 0;
    char *argv[1024];

    int i;
    for(i = 0;i < root->child_count;i++){
        argv[argc++] = root->child_vector[i]->token;
    }
    
    argv[argc] = NULL;
    execvp(argv[0], argv);
    perror("exec");
    exit(1);
}
```



#### tree_execute_pipe

用来执行管道命令。

该节点必定含有两个孩子节点，所以将两个孩子节点取出，并利用dup2函数创建一个管道，使得左孩子节点的输出成为右孩子节点的输入，再分别执行左右孩子命令。

```c
void tree_execute_pipe(tree* root)
{
    int fd[2];
    pid_t pid;
    tree *left = root->child_vector[0];
    tree *right = root->child_vector[1];
    
    pipe(fd);
    pid = fork();
    if(pid == 0){
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        tree_execute(left);
        // exit(1);
    }
    dup2(fd[0], 0);
    close(fd[0]);
    close(fd[1]);
    tree_execute(right);
    // exit(1);
}
```



#### tree_execute_async

用来执行后台命令，只会有一个孩子节点，其中就是命令去除掉&的部分。

```C
void tree_execute_async(tree *root)
{
    tree *body = root->child_vector[0];
    tree_execute(body);
}
```



#### tree_execute

判断当前节点的类型，并执行相应的命令

```c
void tree_execute(tree_t *this)
{
    switch (this->type) {
        case TREE_ASYNC:
            tree_execute_async(this); 
            break;

        case TREE_PIPE:
            tree_execute_pipe(this); 
            break;

        case TREE_REDIRICT:
            tree_execute_redirect(this); 
            break;

        case TREE_BASIC:
              tree_execute_basic(this); 
            break;
    }
}
```



#### tree_execute_wrapper

是执行整个命令的入口，如果是cd/exit/pwd命令则直接执行，否则从根节点开始执行语法树，另外如果根节点不是后台命令，则不需要等待子进程执行完语法树。

执行过程需要由子进程来执行，否则父进程在执行完命令后会退出程序（因为execvp函数）。

```c
void tree_execute_wrapper(tree_t *this)
{
    if (tree_execute_builtin(this))
        return;

    int status;
    pid_t pid = fork();
    if (pid == 0) {
        tree_execute(this);
        exit(EXIT_FAILURE);
    }
   
    // cc a-large-file.c &
    if (this->type != TREE_ASYNC)
        wait(&status);
}
```



## 运行结果

```
> echo abc | wc -l &
> 1
```



# job7/pi2.c

## 题目要求

使用N个线程根据莱布尼兹级数计算PI

- 主线程创建N个辅助线程
- 每个辅助线程计算一部分任务，并将结果返回
- 主线程等待N个辅助线程运行结束，将所有辅助线程的结果累加
- 本题要求
  - 使用线程参数，消除程序中的代码重复
  - 不能使用全局变量存储线程返回值



## 解决思路

根据题意

主线程（即main函数）负责创建N个子线程，并等待N个子线程运行结束，将所有辅助线程的结果累加。创建子线程时，需要为其分配计算区间。

子线程负责计算相应区间内的莱布尼兹级数。



## 关键代码

### param结构体

用来表示子线程的计算区间

```c
typedef struct param{
    int start, end;
}param;
```



### result结构体

用来存储线程的计算结果

```C
typedef struct result{
    float sum;
}result;
```



### calculate函数

子线程通过arg参数获取计算区间，并将结果返回。结果由主线程接收。

```c
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
```



### 主线程

主线程负责创建N个子线程，并等待N个子线程运行结束，将所有辅助线程的结果累加。创建子线程时，需要为其分配计算区间。

输入命令时需要加入莱布尼兹级数n以及线程个数thread_num

```c
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
```



## 运行结果

```
./pi2 10000 4
3.141493
```



# job8/pc.c

## 题目要求

```
+ 系统中有3个线程：生产者、计算者、消费者
+ 系统中有2个容量为4的缓冲区：buffer1、buffer2
+ 生产者
  - 生产'a'、'b'、'c'、‘d'、'e'、'f'、'g'、'h'八个字符
  - 放入到buffer1
  - 打印生产的字符
+ 计算者
  - 从buffer1取出字符
  - 将小写字符转换为大写字符，按照 input:OUTPUT 的格式打印
  - 放入到buffer2
+ 消费者
  - 从buffer2取出字符
  - 打印取出的字符
+ 程序输出结果(实际输出结果是交织的)
a
b
c
...
    a:A
    b:B
    c:C
    ...
        A
        B
        C
        ...
```



## 解决思路

建立两个生产者-消费者关系：

- 生产者和计算者，其中计算者是"消费者"
- 计算者和消费者，其中计算者是"生产者"

因此需要创建两个缓冲区、两个互斥量以及两对条件变量（empty_wait 和 full_wait）



## 关键代码

### container_t结构体

用来表示一个生产者-消费者关系的缓冲区（存放数据的数组、数组大小、读指针和写指针）

```C
typedef struct {
    int capacity;
    int *buffer;
    int in;
    int out;
} container_t;
```



### consume函数

消费过程：

先加锁，判断计算者-消费者缓冲区是否为空

- 若为空则解锁并将线程阻塞在wait_full_buffer_cc条件变量上
- 不为空则取出一个元素并输出，然后唤醒一个阻塞在wait_empty_buffer_cc条件变量上的计算者线程，然后解锁

```c
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
```



### produce函数

生产过程：

先加锁，判断生产者-计算者缓冲区是否为满

- 若为满则解锁并将线程阻塞在wait_empty_buffer_pc条件变量上
- 不为满则放置一个元素并输出并放置在生产者-计算者缓冲区，然后唤醒一个阻塞在wait_full_buffer_pc条件变量上的计算者线程，然后解锁

```c
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
```



### compute函数

计算过程：

先加锁，判断生产者-计算者缓冲区是否为空

- 若为空则解锁并将线程阻塞在wait_full_buffer_pc条件变量上
- 不为空则取出一个元素并输出对应的大写，然后唤醒一个阻塞在wait_empty_buffer_pc条件变量上的生产者线程，然后解锁

再加锁，判断计算者-消费者缓冲区是否为满

- 若为满则解锁并将线程阻塞在wait_empty_buffer_cc条件变量上
- 不为满则取出一个元素并转换成对应的大写，将结果放置在计算者-消费者缓冲区，然后唤醒一个阻塞在wait_full_buffer_cc条件变量上的消费者线程，然后解锁

```c
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
```



## 运行结果

```
a
b
c
    a:A
    b:B
    c:C
d
e
f
        A
        B
        C
    d:D
    e:E
    f:F
g
h
        D
        E
        F
    g:G
    h:H
        G
        H
```





# job9/pc.c

## 题目要求

使用信号量解决生产者、计算者、消费者问题



## 解决思路

建立两个生产者-消费者关系：

- 生产者和计算者，其中计算者是"消费者"
- 计算者和消费者，其中计算者是"生产者"

因此需要创建两个缓冲区、两个互斥量以及两对条件变量（empty_wait 和 full_wait）



另外，在执行判断缓冲区是否为空/满、加锁、解锁、阻塞进程这些操作时，封装成PV操作。

## 关键代码

### container_t结构体

用来表示一个生产者-消费者关系的缓冲区（存放数据的数组、数组大小、读指针和写指针）

```C
typedef struct {
    int capacity;
    int *buffer;
    int in;
    int out;
} container_t;
```



### sema_t结构体

value表示某个资源的资源量，为正表示有多少个资源还可以被访问，为负则表示有多少个线程还试图访问该资源

mutex 和 cond分别是锁和条件变量

```C
typedef struct {
    int value;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} sema_t;
```

以下变量的类型均为该结构体：

mutex_sema_pc：生产者-计算者锁

mutex_sema_cc：计算者-消费者锁

这两个资源的资源量初始均为1，表示允许1个线程进行访问



empty_buffer_sema_pc：生产者-计算者的empty条件变量

empty_buffer_sema_cc：计算者-消费者的empty条件变量

这两个资源的资源量初始均为对应缓冲区的大小 - 1，即value = 3。表示每个缓冲区均有3个空闲空间。



full_buffer_sema_pc：生产者-计算者的full条件变量

full_buffer_sema_cc：计算者-消费者的full条件变量

这两个资源的资源量初始均为0，表示每个缓冲区内没有元素。



### sema_wait函数

实现P操作。

先加锁，判断是否有剩余资源可以访问

- 若有则令资源量-1，并解锁
- 若没有则阻塞线程并解锁

```C
void sema_wait(sema_t *sema)
{
    pthread_mutex_lock(&sema->mutex);
    while (sema->value <= 0)
        pthread_cond_wait(&sema->cond, &sema->mutex);
    sema->value--;
    pthread_mutex_unlock(&sema->mutex);
}
```



### sema_signal函数

实现V操作。

先加锁，令资源量+1，并唤醒该资源上的一个线程，最后解锁

```c
void sema_signal(sema_t *sema)
{
    pthread_mutex_lock(&sema->mutex);
    ++sema->value;
    pthread_cond_signal(&sema->cond);
    pthread_mutex_unlock(&sema->mutex);
}
```



### consume函数

消费过程：

先访问计算者-消费者缓冲区是否有full资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

再访问计算者-消费者锁是否有资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

当两个资源都占有时，获取计算者-消费者缓冲区中的元素并输出

```c
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
```



### produce函数

生产过程：

先访问生产者-计算者缓冲区是否有empty资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

再访问生产者-计算者锁是否有资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

当两个资源都占有时，将元素放置到生产者-计算者缓冲区中并输出

```c
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
```



### compute函数

计算过程：

先访问生产者-计算者缓冲区是否有full资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

再访问生产者-计算者锁是否有资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

当两个资源都占有时，获取生产者-计算者缓冲区中的元素转换成大写并输出



访问计算者-消费者缓冲区是否有empty资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

再访问计算者-消费者锁是否有资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

当两个资源都占有时，将大写元素放置到计算者-消费者缓冲区中并输出

```c
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
```



## 运行结果

```
a
b
c
    a:A
    b:B
    c:C
d
e
f
        A
        B
        C
    d:D
    e:E
    f:F
g
h
        D
        E
        F
    g:G
    h:H
        G
        H
```



# job10/pfind.c

## 题目要求

- 要求使用多线程完成
  - 主线程创建若干个子线程
  - 主线程负责遍历目录中的文件
  - 遍历到目录中的叶子节点时，将叶子节点发送给子线程进行处理

- 两者之间使用生产者消费者模型通信
  - 主线程生成数据
  - 子线程读取数据



## 解决思路

主线程

- 主线程创建N子线程
- 主线程负责遍历目录中的文件，将文件路径放置在任务队列中。
- 当目录中的文件全部遍历完后，添加N个特殊任务，表示主线程已经完成递归遍历，不会再向任务队列中放置任务，子线程可以退出。



子线程

- 子线程在一个循环中
- 子线程从任务队列中，获取一个任务并执行
- 当读取到一个特殊的任务，循环结束



两者之间使用生产者消费者模型通信

- 主线程生成数据

- 子线程读取数据



## 关键代码

### Task结构体

is_end表示该任务是否为特殊任务，如果是则为1，否则为0

path表示该任务的路径，即文件路径

string表示该任务的目标，即所要查询的字符串

```c
typedef struct Task {
    int is_end;
    char path[128];
    char string[128];
}task;
```



### find_file函数

用来打开文件并查询该文件中是否含有target字符串

```C
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
```



### worker_entry函数

子线程的函数，子线程作为消费者

消费过程：

先访问任务队列中是否有full资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

再访问锁是否有资源可以访问

- 若有则占有资源
- 若没有则阻塞并解锁

当两个资源都占有时，获取任务队列中的元素：

- 如果是特殊任务，则退出循环
- 否则查找该任务中是否存在目标字符串

```C
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
```





### produce函数

主线程的函数，主线程作为生产者

生产过程：

先遍历当前目录中的文件/文件夹

- 如果是文件夹，则递归遍历

- 如果是文件

  先访问任务队列中是否有empty资源可以访问

  - 若有则占有资源

  - 若没有则阻塞并解锁

  再访问锁是否有资源可以访问

  - 若有则占有资源

  - 若没有则阻塞并解锁

当两个资源都占有，且当前遍历到的是文件，则将其转换为任务并放置到队列中。



当遍历完目录中的所有文件/文件夹时：

先访问任务队列中是否有empty资源可以访问

- 若有则占有资源

- 若没有则阻塞并解锁

再访问锁是否有资源可以访问

- 若有则占有资源

- 若没有则阻塞并解锁

当两个资源都占有时，向任务队列中添加N个特殊任务

```C
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
```



## 运行结果

```
./pfind test main

test/hello.c: int main(int argc, char *argv[])
test/world.c: int main(int argc, char *argv[])
```

