#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "command.h"

static void get_full_path(char *path)
{
    if (path[0] == '/') {
        return;
    }

    char tmp[1000];
    getcwd(tmp, 1000);
    strcat(tmp, "/");
    strcat(tmp, path);
    strcpy(path, tmp);

}

int cmd_lpwd(char *path)
{
    if (getcwd(path, 1000) == NULL) {
        return -1;
    }
    printf("%s\n", path);

    return 0;
}

int cmd_lcd(char *dir)
{
    return chdir(dir);
}

int cmd_lmkdir(char *dir)
{
    return mkdir(dir, S_IRWXU | S_IRWXG | S_IRWXO);
}

int cmd_dir(char *arg, char *result)
{
    DIR *dp;
    struct dirent *entry;
    struct stat statbuf;

    result[0] = 0;
    if ((dp = opendir(arg)) == NULL) {
        return -1;
    }
    chdir(arg);
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] != '.') {
            strcat(result, entry->d_name);
            strcat(result, " ");
        }
    }

    printf("%s\n", result);
    return 0;
}
