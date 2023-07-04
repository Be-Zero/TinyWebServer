// deadlock1.cpp
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
int a = 0;
int b = 0;
pthread_mutex_t mutex_a;
pthread_mutex_t mutex_b;
void *another(void *arg) // 新线程运行的函数
{
    pthread_mutex_lock(&mutex_b); // 加锁b
    printf("in child thread, got mutex b, waiting for mutex a\n");
    sleep(5);
    ++b;
    pthread_mutex_lock(&mutex_a); // 加锁a
    b += a++;
    pthread_mutex_unlock(&mutex_a); // 释放a
    pthread_mutex_unlock(&mutex_b); // 释放b
    pthread_exit(NULL);             // 结束线程
}
int main()
{
    pthread_t id;                             // 定义线程id
    pthread_mutex_init(&mutex_a, NULL);       // 初始化互斥锁
    pthread_mutex_init(&mutex_b, NULL);       // 初始化互斥锁
    pthread_create(&id, NULL, another, NULL); // 创建线程，1id，2设置新线程的属性（默认属性），34指定新线程运行的函数及其参数
    pthread_mutex_lock(&mutex_a);             // 加锁a
    printf("in parent thread, got mutex a, waiting for mutex b\n");
    sleep(5);
    ++a;
    pthread_mutex_lock(&mutex_b); // 加锁b
    a += b++;
    pthread_mutex_unlock(&mutex_b);  // 释放b
    pthread_mutex_unlock(&mutex_a);  // 释放a
    pthread_join(id, NULL);          // 回收线程，1目标线程标识符，2退出信息
    pthread_mutex_destroy(&mutex_a); // 销毁a
    pthread_mutex_destroy(&mutex_b); // 销毁b
    return 0;
}