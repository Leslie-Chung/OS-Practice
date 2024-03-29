#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
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

void find_dir(char *path, char *target)
{
    DIR *dir = opendir(path);
    struct dirent *entry;
    char pathstr[256];
    sprintf(pathstr, "%s/", path);
    // puts(pathstr);
    while (entry = readdir(dir)) {
        if (strcmp(entry->d_name, ".") == 0)
            continue;
        else if (strcmp(entry->d_name, "..") == 0)
            continue;
        else if (entry->d_type == DT_DIR) {
            // printf("dir %s\n", entry->d_name);
            char dirstr[512];
            sprintf(dirstr, "%s%s", pathstr, entry->d_name);
            find_dir(dirstr, target);
        }           
        else if (entry->d_type == DT_REG) {
            // printf("file %s\n", entry->d_name);
            char dirstr[512];
            sprintf(dirstr, "%s%s", pathstr, entry->d_name);
            find_file(dirstr, target);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[])
{
    if (argc != 3) {
        puts("Usage: sfind file string");
        return 0;
    }
    char *path = argv[1];
    char *string = argv[2];
    struct stat info;
    stat(path, &info);
    if (S_ISDIR(info.st_mode))
        find_dir(path, string);
    else
        find_file(path, string);
    return 0;
}
