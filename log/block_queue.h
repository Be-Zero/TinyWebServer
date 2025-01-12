/*************************************************************
 *循环数组实现的阻塞队列，m_back = (m_back + 1) % m_max_size;
 *线程安全，每个操作前都要先加互斥锁，操作完后，再解锁
 **************************************************************/

#ifndef BLOCK_QUEUE_H
#define BLOCK_QUEUE_H

#include <iostream>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include "../lock/locker.h"
using namespace std;

// 模板化
template <class T>
class block_queue
{
public:
    // 构造函数
    block_queue(int max_size = 1000)
    {
        // 初始化私有成员
        if (max_size <= 0)
        {
            exit(-1);
        }

        // 构造函数创建循环数组
        m_max_size = max_size;
        m_array = new T[max_size];
        m_size = 0;
        m_front = -1;
        m_back = -1;
    }

    // 清空队列
    void clear()
    {
        m_mutex.lock();
        m_size = 0;
        m_front = -1;
        m_back = -1;
        m_mutex.unlock();
    }

    // 析构函数
    ~block_queue()
    {
        m_mutex.lock();
        if (m_array != NULL)
            delete[] m_array;

        m_mutex.unlock();
    }
    // 判断队列是否满了
    bool full()
    {
        m_mutex.lock();
        if (m_size >= m_max_size)
        {

            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    // 判断队列是否为空
    bool empty()
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return true;
        }
        m_mutex.unlock();
        return false;
    }
    // 返回队首元素
    bool front(T &value)
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_front];
        m_mutex.unlock();
        return true;
    }
    // 返回队尾元素
    bool back(T &value)
    {
        m_mutex.lock();
        if (0 == m_size)
        {
            m_mutex.unlock();
            return false;
        }
        value = m_array[m_back];
        m_mutex.unlock();
        return true;
    }

    // 获取大小
    int size()
    {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_size;

        m_mutex.unlock();
        return tmp;
    }

    // 获取上限
    int max_size()
    {
        int tmp = 0;

        m_mutex.lock();
        tmp = m_max_size;

        m_mutex.unlock();
        return tmp;
    }
    // 往队列添加元素，需要将所有使用队列的线程先唤醒
    // 当有元素push进队列,相当于生产者生产了一个元素
    // 若当前没有线程等待条件变量,则唤醒无意义
    bool push(const T &item)
    {
        // 当前大小达到上限，唤醒所有线程工作，添加失败
        m_mutex.lock();
        if (m_size >= m_max_size)
        {

            m_cond.broadcast();
            m_mutex.unlock();
            return false;
        }
        // 插入队列
        m_back = (m_back + 1) % m_max_size;
        m_array[m_back] = item;

        m_size++;
        // 唤醒
        m_cond.broadcast();
        m_mutex.unlock();
        return true;
    }

    // pop时,如果当前队列没有元素,将会等待条件变量
    bool pop(T &item)
    {
        m_mutex.lock();
        // 当前队列没有元素，等待
        while (m_size <= 0)
        {
            // 等待mutex
            if (!m_cond.wait(m_mutex.get()))
            {
                m_mutex.unlock();
                return false;
            }
        }
        // 更新头指针和大小
        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

    // 增加了超时处理
    bool pop(T &item, int ms_timeout)
    {
        // timespec提供秒和纳秒，timeval提供秒和微妙
        struct timespec t = {0, 0};
        struct timeval now = {0, 0};
        // 获取当前时间
        gettimeofday(&now, NULL);
        m_mutex.lock();
        if (m_size <= 0)
        {
            // 更新到期时间
            t.tv_sec = now.tv_sec + ms_timeout / 1000;
            t.tv_nsec = (ms_timeout % 1000) * 1000;
            // 超时等待
            if (!m_cond.timewait(m_mutex.get(), t))
            {
                m_mutex.unlock();
                return false;
            }
        }

        if (m_size <= 0)
        {
            m_mutex.unlock();
            return false;
        }

        m_front = (m_front + 1) % m_max_size;
        item = m_array[m_front];
        m_size--;
        m_mutex.unlock();
        return true;
    }

private:
    locker m_mutex; // 互斥锁
    cond m_cond;    // 条件变量
    T *m_array;     // 循环数组
    int m_size;     // 大小
    int m_max_size; // 上限
    int m_front;    // 头元素
    int m_back;     // 尾元素
};

#endif
