#include<stdio.h>
#include<string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#define MAX_CHILD 10
enum {
    TREE_BACK,
    TREE_PIPE,
    TREE_REDIRICT,
    TREE_BASIC,
    TREE_TOKEN,
};

typedef struct Tree{
    int type;
    char *token;
    int child_count;
    struct Tree *child_vector[MAX_CHILD];
}tree;
int tex_type(char *word){
    int i;
    if(!strncmp(word, ">>", 2) || !strncmp(word, ">", 1) || !strncmp(word, "<", 1)) 
        return TREE_REDIRICT;
    else if(!strncmp(word, "|", 1))
        return TREE_PIPE;
    else if(!strncmp(word, "&", 1))
        return TREE_BACK;
    return TREE_TOKEN;
}

int split(char *line, const char *separator, char *word_array[])
{
    int word_count = 0;
    char *word;
    int len = strlen(line);
    line[len - 1] = '\0';
    word = strtok(line, separator);
    while (word != NULL) {
        word_array[word_count++] = word;
        word = strtok(NULL, separator);
    }
    
    return word_count;
}

tree* create_node(char *word){
    tree *cur = (tree *) malloc(sizeof(tree));     
    cur->type = tex_type(word);
    cur->token = word;
    cur->child_count = 0;
    return cur;
}


tree* parse_command(char *command){
    int wordc;
    char *wordv[1024];
    wordc = split(command, " ", wordv);
    
    int i;
    tree *root, *parent, *cur;
    cur = root = parent = NULL;

    int next_type_is_basic = 1;
    for(i = 0;i < wordc;i++){
        if(next_type_is_basic == 0){ // 如果下一个节点不应该是basic，即当前节点是basic
            cur = create_node(wordv[i]);
            if(cur->type != TREE_TOKEN){
                parent = cur;
                cur->child_vector[cur->child_count++] = root;
                root = cur;
                if(cur->type == TREE_BACK)
                    cur->token = "back";
                else if(cur->type == TREE_PIPE)
                    cur->token = "pipe";
                else
                    cur->token = "redirect";

                if(cur->type != TREE_REDIRICT)
                    next_type_is_basic = 1;
                else{
                    tree *p, *q;
                    if(!strncmp(wordv[i], ">>", 2)){
                        p = create_node(">>");
                        q = create_node(wordv[i] + 2);
                    } else if(!strncmp(wordv[i], ">", 1)){
                        p = create_node(">");
                        q = create_node(wordv[i] + 1);
                    }
                    else {
                        p = create_node("<");
                        q = create_node(wordv[i] + 1);
                    }
                    p->type = TREE_TOKEN;
                    cur->child_vector[cur->child_count++] = p;
                    p->type = TREE_TOKEN;
                    cur->child_vector[cur->child_count++] = q;

                }
            }
            else
                parent->child_vector[parent->child_count++] = cur;
        }
        else{
            next_type_is_basic = 0;
            cur = create_node("basic");
            cur->type = TREE_BASIC;
            if(root == NULL)
                root = cur;
            else
                parent->child_vector[parent->child_count++] = cur;
            
            parent = cur;
            i--;
        }
    }
    return root;
}

int verbose = 0;
void dfs(tree* root, int tabscount, int destroy){
    // if(root == NULL) return ;
    int i;
    for(i = 0;i < tabscount && verbose;i++)
        printf(" ");
    if(verbose)
        puts(root->token);
    for(i = 0;i < root->child_count;i++){
        dfs(root->child_vector[i], tabscount + 2, destroy);
        if(destroy)
            free(root->child_vector[i]); 
    }
}

int tree_execute_builtin(tree *root)
{
    if(root->type != TREE_BASIC)
        return 0;
    tree *child0 = root->child_vector[0];
    if(strcmp(child0->token, "exit") == 0){
        exit(0);
        // return 1;
    }

    if(strcmp(child0->token, "pwd") == 0){
        char buf[1024];
        getcwd(buf, sizeof(buf));
        puts(buf);
        return 1;
    }

    if(strcmp(child0->token, "cd") == 0){
        if(root->child_count == 1)
            return 1;
        tree *child1 = root->child_vector[1];
        int error = chdir(child1->token);
        if(error < 0)
            perror("cd");
        return 1;
    }

    return 0;
}

void tree_execute(tree *);

void tree_execute_async(tree *root)
{
    tree *body = root->child_vector[0];
    tree_execute(body);
}

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

void tree_execute(tree *root)
{
    switch (root->type) {
        case TREE_BACK:
            tree_execute_async(root); 
            break;

        case TREE_PIPE:
            tree_execute_pipe(root); 
            break;

        case TREE_REDIRICT:
            tree_execute_redirect(root); 
            break;

        case TREE_BASIC:
            tree_execute_basic(root); 
            break;
    }
}

void tree_execute_wrapper(tree *root)
{
    if (tree_execute_builtin(root)) // 如果是cd/exit/pwd命令
        return;

    int status;
    pid_t pid = fork();
    if (pid == 0) {
        tree_execute(root);
        exit(1); //正常情况下，不会执行到这一句话
    }
   
    if (root->type != TREE_BACK)
        wait(&status);
}

void mysys(char *command){
    tree *root = parse_command(command);
    // dfs(root, 0, 0);
    if(root != NULL){
        tree_execute_wrapper(root);
        dfs(root, 0, 1);
    }
    
}

int main(int argc, char *argv[]){
    char buf_read[1024];
	char input_message[] = "> ";
    if(argc == 2 && strcmp(argv[1], "-v") == 0)
        verbose = 1;
    while(1) {
		memset(buf_read, 0, sizeof(buf_read));
		// 清空写入文件缓冲区中的内存
		
		// 打印提示信息
		write(STDOUT_FILENO, input_message, strlen(input_message));
		// 读取用户的键盘输入信息
		read(STDIN_FILENO, buf_read, sizeof(buf_read));
		// 判断用户输入的内容是否为quit
        mysys(buf_read);
    }	    
}
