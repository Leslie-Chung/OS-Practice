#include<sys/types.h>
#include<dirent.h>
#include<stdio.h>
#include<string.h>
int main(int argc, char *argv[])
{
    DIR *dirptr = NULL;
    struct dirent *entry;
    if (argc == 1) argv[1] = "./";

    dirptr = opendir(argv[1]);
    if (dirptr != NULL)
    {
        while(1)
        {
            entry = readdir(dirptr);
            if (entry == NULL)
                break;
            switch (entry->d_type) 
            {
                case DT_DIR:
                    if(!strncmp(entry->d_name, ".", 1) || !strncmp(entry->d_name, "..", 2))
                        break;
                default:
                    // stack[++top] = entry->d_name;
                    puts(entry->d_name);
            }
        }
    } else 
        puts("No Such File");
    
    closedir(dirptr);
    return 0;
}
