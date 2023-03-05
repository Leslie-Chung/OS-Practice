#include<stdio.h>
#include<unistd.h>
int main(int argc, char *argv[])
{
    execvp("echo", argv);
    return 0;
}
