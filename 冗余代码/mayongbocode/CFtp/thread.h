//
// Created by yobol on 16-11-9.
//
#include <stdio.h>
#include <pthread.h>

/**
 * 创建一个新线程
 *
 * @param thread 指向pthread_t类型数据的指针
 * @param func 成功创建线程后调用的函数
 * @param arg 被调用函数的参数
 * @return 创建成功返回0；失败则返回-1
 */
int
createNewThread(pthread_t *thread,void * (*func)(void *),void *arg)
{
    /*
     * 第二个参数为线程属性
     */
    if (pthread_create(thread,NULL,func,arg) == 0)
    {
        return 0;
    }
    else
    {
        printf("server > Fail to create new thread\n");
        return -1;
    }
}

/**
 * 挂起主线程，等待其他非主线程运行完毕后，再执行此方法后的代码
 *
 * @param thread 主线程要等待的非主线程
 * @return 成功返回0；失败则返回-1
 */
int
joinThread(pthread_t thread,void *threadResult)
{
    /*
     * 第二个参数为thread指向的线程，调用pthread_exit(void *retval)方法的void *类型的参数
     */
    if (pthread_join(thread,threadResult) == 0)
    {
        return 0;
    }
    else
    {
        printf("server > Fail to join the thread\n");
        return -1;
    }
}

/**
 * 释放进程所占用的空间
 *
 * @param thread 要销毁的进程
 * @return 成功返回0；失败则返回-1
 */
int
detachThread(pthread_t thread)
{
    if (pthread_detach(thread) == 0)
    {
        return 0;
    }
    else
    {
        printf("server > Fail to detach the thread\n");
        return -1;
    }
}

/**
 * 创建并初始化互斥量
 *
 * @param mutex 保存新创建的互斥量的容器
 * @return
 */
int
createMutex(pthread_mutex_t *mutex)
{
    if (!mutex && pthread_mutex_init(mutex,NULL) != 0)
    {
        printf("Fail to create mutex\n");
        return -1;
    }
    else
    {
        return 0;
    }
}

/**
 * 互斥量加锁
 *
 * @param mutex
 * @return
 */
int
lockMutex(pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) == 0)
    {
        return 0;
    }
    else
    {
        printf("Fail to lock for mutex\n");
        return -1;
    }
}

/**
 * 互斥量解锁
 *
 * @param mutex
 * @return
 */
int
unlockMutex(pthread_mutex_t *mutex)
{
    if (pthread_mutex_unlock(mutex) == 0)
    {
        return 0;
    }
    else
    {
        printf("Fail to unlock for mutex\n");
        return -1;
    }
}

/**
 * 销毁互斥量WWWWW
 *
 * @param mutex
 * @return
 */
int
destroyMutex(pthread_mutex_t *mutex)
{
    if (pthread_mutex_destroy(mutex) == 0)
    {
        return 0;
    }
    else
    {
        printf("Fail to destroy mutex\n");
        return -1;
    }
}
