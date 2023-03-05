#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc, char *argv[])
{
    int fd = open(argv[1], O_RDONLY);
    char buffer[1024];
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        int size = read(fd, buffer, sizeof(buffer));
        if(size == 0)
            break;
		write(STDOUT_FILENO, buffer, strlen(buffer));
    }
    close(fd);
    return 0;
}
