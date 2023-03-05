#include<sys/types.h>
#include<dirent.h>
#include<stdio.h>
#include<string.h>

void getchdir(char *arg, int tabnum)
{
    // puts(arg);
    DIR *dirptr = NULL;
    struct dirent *entry;
    dirptr = opendir(arg);
    if (dirptr != NULL)
    {
        int i;
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
                    for(i = 0;i < tabnum;i++) printf(" ");
                    puts(entry->d_name);
                    strcat(arg, "/");
                    strcat(arg, entry->d_name);
                    getchdir(arg, tabnum + 1);                      
                    break;
                default:
                    for(i = 0;i < tabnum;i++) printf(" ");
                    puts(entry->d_name);
            }
        }
    } else
        puts("No Such File");


    closedir(dirptr);


}
int main(int argc, char *argv[])
{
    if (argc == 1) argv[1] = "./";
    
    getchdir(argv[1], 0);

    return 0;
}
