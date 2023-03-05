#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int main(int argc, char *argv[])
{
    int fd = open(argv[1], O_RDONLY);
    int fw = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC, 00777);
    char buffer[1024];
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        int size = read(fd, buffer, sizeof(buffer));
        if(size == 0)
            break;
		write(fw, buffer, strlen(buffer));
    }
    close(fd);
    close(fw);
    return 0;
}
