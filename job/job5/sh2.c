#include<stdio.h>
#include <string.h>
#include <unistd.h>
#include<stdlib.h>
#include<sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
struct command {
    int argc;
    char *argv[10];
    char *input;
    char *output;
    char *append;
};
int split(char *line, const char *separator, char *word_array[])
{
    int word_count = 0;
    char *word;

    word = strtok(line, separator);
    while (word != NULL) {
        word_array[word_count++] = word;
        word = strtok(NULL, separator);
    }
    
    return word_count;
}

void parse_command(char *line, struct command *Command)
{
    int wordc;
    char *wordv[10];

    wordc = split(line, " ", wordv);
    int i;
    int j = 0;
    for(i = 0;i < wordc;i++)
    {
        if(!strncmp(wordv[i], ">>", 2))
        {
            Command->append = wordv[i] + 2;
            Command->output = NULL;
        } else if(!strncmp(wordv[i], ">", 1))
        {
            Command->output = wordv[i] + 1;
            Command->append = NULL;
        } else if(!strncmp(wordv[i], "<", 1))
            Command->input = wordv[i] + 1;
        else
            Command->argv[j++] = wordv[i];
    }
    Command->argc = j;
    Command->argv[j] = NULL;
}

void mysys(char *commands)
{
    commands[strlen(commands) - 1] = '\0';
    struct command Command;
    parse_command(commands, &Command);
    pid_t pid = fork();
    int fd;
    if (pid == 0) {
        if (Command.input != NULL) {
            fd = open(Command.input, O_RDONLY);
            dup2(fd, 0);
        }
        if (Command.output != NULL) {
            fd = open(Command.output, O_WRONLY | O_CREAT | O_TRUNC, 00777);
            dup2(fd, 1);
        }
        if (Command.append != NULL) {
            fd = open(Command.append, O_WRONLY | O_APPEND, 00777);
            dup2(fd, 1);
        }
        
        execvp(Command.argv[0], Command.argv);
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
        else if(!strncmp(buf_read, "cd", 2))
        {
            buf_read[strlen(buf_read) - 1] = '\0';
            chdir(buf_read + 3);
        } else
            mysys(buf_read);
    }	    
}

