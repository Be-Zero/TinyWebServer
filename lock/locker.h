#ifndef LOCKER_H
#define LOCKER_H

#include <exception>
#include <pthread.h>
#include <semaphore.h>

// 信号量，用于同步线程对共享数据的访问
class sem
{
public:
    // 构造函数
    sem()
    {
        if (sem_init(&m_sem, 0, 0) != 0)
        {
            throw std::exception();
        }
    }
    sem(int num)
    {
        if (sem_init(&m_sem, 0, num) != 0)
        {
            throw std::exception();
        }
    }

    // 析构函数
    ~sem()
    {
        sem_destroy(&m_sem);
    }

    // P操作
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    // V操作
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }

private:
    // 信号量
    sem_t m_sem;
};

// 互斥锁，用于同步线程对共享数据的访问
class locker
{
public:
    // 构造函数
    locker()
    {
        if (pthread_mutex_init(&m_mutex, NULL) != 0)
        {
            throw std::exception();
        }
    }
    // 析构函数
    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }
    // 加锁
    bool lock()
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }
    // 解锁
    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }
    // 获取mutex
    pthread_mutex_t *get()
    {
        return &m_mutex;
    }

private:
    pthread_mutex_t m_mutex;
};

// 条件变量，用于线程之间同步共享数据的值
class cond
{
public:
    // 构造函数
    cond()
    {
        if (pthread_cond_init(&m_cond, NULL) != 0)
        {
            // pthread_mutex_destroy(&m_mutex);
            throw std::exception();
        }
    }
    // 析构函数
    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }
    // P操作
    bool wait(pthread_mutex_t *m_mutex)
    {
        int ret = 0;
        // pthread_mutex_lock(&m_mutex);
        //
        ret = pthread_cond_wait(&m_cond, m_mutex);
        // pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    // 加锁，然后将线程插入等待队列，再解锁并返回
    bool timewait(pthread_mutex_t *m_mutex, struct timespec t)
    {
        int ret = 0;
        // pthread_mutex_lock(&m_mutex);
        ret = pthread_cond_timedwait(&m_cond, m_mutex, &t);
        // pthread_mutex_unlock(&m_mutex);
        return ret == 0;
    }
    // 唤醒一个等待资源的线程
    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }
    // 唤醒所有等待资源的线程
    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    // static pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};
#endif
