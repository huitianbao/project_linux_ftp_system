//
// Created by yobol on 16-11-12.
// fm_开头的都是基础函数
//      /开头表示绝对路径
//      ..或者无/开头表示相对路径
//
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

#define PATH_LEN 128
#define USER_AUTH S_IRUSR | S_IWUSR | S_IXUSR

/**
 * 判断文件是否存在
 *
 * @param fileName
 * @return 存在返回1；不存在返回0
 */
int
isExisted(char *fileName)
{
    FILE *file;

    if ((file = fopen(fileName,"r")) == NULL)
    {
        return 0;
    }
    return 1;
}

/**
 * 显示当前所在目录的路径
 *
 * @param path 当前目录
 */
void
fm_pwd(const char *path)
{
    printf("\tCurrent directory is : %s\n",path);
}

/**
 * 显示当前目录下的所有文件
 *
 * 目录后面要加上一个斜线字符'/'
 *
 * @param path 当前目录
 * @return 目录项
 */
char *
fm_ls(const char *path)
{
    DIR * dir;
    struct dirent *entry;//目录项
    struct stat statbuf;//目录项目状态
    char *tmp = (char *)malloc(1024);

    if ((dir = opendir(path)) == NULL)
    {
        printf("\tFail to open the directory : %s\n",path);
        return NULL;
    }

    chdir(path);//进入指定目录dir
    strcpy(tmp,"\t");
    while((entry = readdir(dir)) != NULL)
    {
        lstat(entry->d_name,&statbuf);
        if (S_ISDIR(statbuf.st_mode))//目录项目是个目录
        {
            if (strcmp(".",entry->d_name) == 0 || strcmp("..",entry->d_name) == 0 || strncmp(entry->d_name,".",1) == 0)
            {
                continue;
            }
            strcat(tmp,entry->d_name);
            strcat(tmp,"/ ");
        }
        else
        {
            if (strncmp(entry->d_name,".",1) == 0)
            {
                continue;
            }
//            printf("%s ",entry->d_name);
            strcat(tmp,entry->d_name);
            strcat(tmp," ");
        }
    }

    if (closedir(dir) == -1)
    {
        printf("\tFail to close dir stream\n");
        return NULL;
    }
    strcat(tmp,"\n");

    return tmp;
}

/**
 * 切换目录
 *
 * @param path 当前目录
 * @param target 要切换到的文件夹; ..为上一级目录; 目录前后都不需要加/
 * @return
 */
char *
fm_cd(char *path,char *target)
{
    DIR *dir;
    struct dirent *entry;//目录项
    struct stat statbuf;
    char *tmp = (char *)malloc(PATH_LEN);
    int tip = 0;

    if (strcmp(target,"..") == 0)
    {
        strcpy(tmp,path);
        chdir(tmp);
        chdir(target);
        getcwd(tmp,PATH_LEN);
        path = tmp;
        tip = 1;
    }
    else if (strncmp(target,"/",1) == 0)//绝对路径
    {
        chdir(target);
        getcwd(tmp,PATH_LEN);
        path = tmp;
        tip = 1;
    }
    else
    {
        if (chdir(path) != 0)
        {
            printf("\tFail to choose directory\n");
        }
        strcpy(tmp,path);
        if (strlen(path) > 1)//是否为根目录，1为是根目录,大于1不是根目录
        {
            strcat(tmp,"/");
        }
        strcat(tmp,target);
        lstat(tmp,&statbuf);

        if (S_ISDIR(statbuf.st_mode))//是个目录
        {

            dir = opendir(path);
            //判断该目录是否属于path路径下
            while((entry = readdir(dir)) != NULL)
            {
                if (strcmp(entry->d_name,target) == 0)
                {
                    path = tmp;
                    chdir(path);
                    tip = 1;
                }
            }
            closedir(dir);
        }
    }
    if (tip == 0)
    {
        printf("\tThe \"%s\" directory does't exist in \"%s\" directory\n",target,path);
    }

    return path;
}

/**
 * 在指定目录path中创建目标目录target
 *
 * @param path
 * @param mode
 * @return
 */
void
fm_mkdir(const char *path,const char *target,mode_t mode)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char *tmp = (char *)malloc(PATH_LEN);

    if (strncmp(target,"/",1) != 0)//相对路径
    {
        if (chdir(path) != 0)
        {
            printf("\tFail to choose directory\n");
            return;
        }
        if ((dir = opendir(path)) != NULL)
        {
            //判断该目录是否属于path路径下
            while((entry = readdir(dir)) != NULL)
            {
                if (strcmp(entry->d_name,target) == 0)
                {
                    printf("\tThe \"%s\" directory existed\n",target);
                    return;
                }
            }
            closedir(dir);
        }
    }

    if (mkdir(target,mode) == -1)
    {
        printf("\tFail to create \"%s\" directory\n",target);
    }
}

/**
 * 递归删除目录项
 */
void
dfs_remove_dir()
{
    DIR *cur_dir = opendir(".");
    struct dirent *ent = NULL;
    struct stat st;

    if (!cur_dir)
    {
        return;
    }

    while ((ent = readdir(cur_dir)) != NULL)
    {
        stat(ent->d_name, &st);

        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
        {
            continue;
        }

        if (S_ISDIR(st.st_mode))
        {
            chdir(ent->d_name);
            dfs_remove_dir();
            chdir("..");
        }

        remove(ent->d_name);
    }

    closedir(cur_dir);
}

/**
 * 删除目录
 *
 * @param path_raw
 * @return 成功返回0;失败返回-1
 */
int
remove_dir(const char *path)
{
    char *temp = (char *)malloc(PATH_LEN);

    strcpy(temp,path);

    if (chdir(temp) == -1)
    {
        return -1;
    }

    dfs_remove_dir();

    chdir("..");
    return rmdir(temp);
}

/**
 * 在指定目录path中删除目标目录target
 *
 * @param path
 * @return 成功返回1；失败返回-1
 */
int
fm_rmdir(char *path,char *target)
{
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    int flag = 0;/* 在path目录下是否找到要删除的目录 */
    char *mode = (char *)malloc(16);/* 删除选项：在target中以-开头，r表示删除文件夹以及文件夹下的所有内容 */
    char *absolutePath = (char *)malloc(PATH_LEN);

    if (strncmp(target,"-",1) == 0)//判断模式
    {
        strncpy(mode,&target[1],1);

        if (strncmp(mode,"r",1) == 0)//删除文件夹以及文件夹下的所有内容
        {
            strncpy(target,&target[3],strlen(target) - 3);
            target[strlen(target) - 3] = '\0';
        }
    }

    if (strncmp(target,"/",1) != 0)//相对路径
    {
        if (chdir(path) != 0)
        {
            printf("\tFail to choose directory\n");
            return -1;
        }

        if ((dir = opendir(path)) != NULL)
        {
            //判断该目录是否属于path路径下
            while((entry = readdir(dir)) != NULL)
            {
                if (strcmp(entry->d_name,target) == 0)
                {
                    flag = 1;
                    break;
                }
            }
            closedir(dir);
        }
    }
    else//绝对路径
    {
        flag = 2;
    }

    if (flag != 0)
    {
        if (strncmp(mode,"r",1) == 0)//迭代删除
        {
            if (flag == 1)//相对路径
            {
                strcat(absolutePath,path);
                strcat(absolutePath,"/");
                strcat(absolutePath,target);
            }
            //询问是否删除
//            printf("\tThe \"%s\" directory maybe contains children,do you want to delete them all (y/n) : ",absolutePath);
//            char res = getchar();
//            if (res == 'y' || res == 'Y')
//            {
                if (remove_dir(target) == 0)
                {
                    printf("\tDone.\n");
                }
//            }
        }
        else
        {
            if (rmdir(target) == -1)
            {
                printf("\tFail to delete \"%s\" directory,maybe it not exists or contains children\n",target);
                return -1;
            }
        }

    }
    else
    {
        printf("\tNot found the \"%s\" directory in \"%s\" directory\n",target,path);
        return -1;
    }

    return 1;
}