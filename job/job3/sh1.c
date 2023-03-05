#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
void mysys(char *command)
{
    pid_t pid;
    pid = fork(); 
    if(pid == 0){
        command[strlen(command) - 1] = '\0';
        const char s[2] = " ";
        char *token;
        token = strtok(command, s);
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


}
int main()
{
    char buf_read[1024];
	char input_message[] = "> ";
    while(1)
	{
		memset(buf_read, 0, sizeof(buf_read));
		// 清空写入文件缓冲区中的内存
		
		// 打印提示信息
		write(STDOUT_FILENO, input_message, strlen(input_message));
		// 读取用户的键盘输入信息
		read(STDIN_FILENO, buf_read, sizeof(buf_read));
		// 判断用户输入的内容是否为quit
	
        if (!strncmp(buf_read, "exit", 4))
	        exit(0);
        else if(!strncmp(buf_read, "pwd", 3))
            puts(getcwd(NULL, 0));
        else if(!strncmp(buf_read, "cd", 2))
        {
            buf_read[strlen(buf_read) - 1] = '\0';
            chdir(buf_read + 3);
        } else
            mysys(buf_read);
    }	    
}
