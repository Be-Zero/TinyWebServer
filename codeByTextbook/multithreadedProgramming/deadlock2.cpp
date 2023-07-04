// deadlock2.cpp
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>
pthread_mutex_t mutex;
void prepare() {
	pthread_mutex_lock(&mutex); 
} 
void infork() { 
	pthread_mutex_unlock(&mutex); 
} 
// 子线程运行的函数。它首先获得互斥锁mutex，然后暂停5s，再释放该互斥锁
void *another(void *arg)
{
    printf("in child thread, lock the mutex\n");
    pthread_mutex_lock(&mutex); // 对mutex加锁
    sleep(5);
    pthread_mutex_unlock(&mutex); // 主程序正在等待mutex，因此线程无法对其解锁
    return nullptr;
}
int main()
{
    pthread_mutex_init(&mutex, NULL);         // 初始化互斥锁mutex
    pthread_t id;                             // 线程id
    pthread_create(&id, NULL, another, NULL); // 创建线程，调用another运行函数
    // 父进程中的主线程暂停1s，以确保在执行fork操作之前，子线程已经开始运行并获得了互斥变量mutex
    sleep(1);
    // pthread_atfork(prepare, infork, infork);
    int pid = fork(); // 创建子进程
    if (pid < 0)
    {
        pthread_join(id, NULL);        // 回收线程，1目标线程标识符，2退出信息
        pthread_mutex_destroy(&mutex); // 销毁mutex
        return 1;
    }
    else if (pid == 0)
    {
        printf("I am in the child, want to get the lock\n");
        // 子进程从父进程继承了互斥锁mutex的状态，该互斥锁处于锁住的状态，这是由父进程中的子线程执行pthread_mutex_lock引起的
        pthread_mutex_lock(&mutex); // 子线程已加锁，所以该语句被阻塞
        printf("I can not run to here, oop...\n");
        pthread_mutex_unlock(&mutex);
        exit(0);
    }
    else
    {
        wait(NULL);
    }
    pthread_join(id, NULL);
    pthread_mutex_destroy(&mutex);
    return 0;
}
