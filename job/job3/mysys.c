#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
void mysys(char *command)
{
    pid_t pid;
    pid = fork();
    char *str = (char *)malloc(sizeof(char) * strlen(command));
    if(pid == 0){
        strcpy(str, command);

        const char s[2] = " ";
        char *token;
        token = strtok(str, s);
        char *argv[1024];
        int i = 0;
       
        if(!token)
            return;

        while(token != NULL)
        {
            argv[i++] = token;
            token = strtok(NULL, s);
        }
        argv[i] = NULL;
        execvp(argv[0], argv);
        
    }
    wait(NULL);
    free(str);
}

int main()
{
    printf("--------------------------------------------------\n");
    mysys("echo HELLO WORLD");
    printf("--------------------------------------------------\n");
    mysys("ls /");
    printf("--------------------------------------------------\n");
    return 0;
}
