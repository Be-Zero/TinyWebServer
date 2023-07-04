#ifndef LST_TIMER
#define LST_TIMER

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/uio.h>
#include <queue>
#include <vector>
#include <time.h>
#include "../log/log.h"

class util_timer;

// 用户数据
struct client_data
{
    // 客户端socket地址
    sockaddr_in address;

    // socket文件描述符
    int sockfd;

    // 定时器
    util_timer *timer;
};

struct cmp {
    bool operator()(util_timer *a, util_timer *b) const {
        return a->expire > b->expire;
    }
};

// 连接资源结构体成员需要用到定时器类
// 计时器
class util_timer
{
public:
    // 超时时间
    time_t expire;

    // 回调函数
    void (*cb_func)(client_data *);

    // 连接资源
    client_data *user_data;
};

// 升序计时器
class sort_timer_heap
{
public:
    sort_timer_heap();  // 初始化链表
    ~sort_timer_heap(); // 回收链表空间

    // 添加定时器，内部调用私有成员add_timer
    void add_timer(util_timer *timer);

    // 删除计时器
    void del_timer(util_timer *timer);

    // 计时，并处理到期的计时器
    void tick();

private:
    // 定义比较器
    struct cmp {
        bool operator()(util_timer* a, util_timer* b) const {
            return a->expire > b->expire;
        }
    };

    // 存储计时器的动态数组
    vector<util_timer*> heap;
};

// 计时器工具包
class Utils
{
public:
    Utils() {}
    ~Utils() {}

    // 设置最小超时单位
    void init(int timeslot);

    // 对文件描述符设置非阻塞
    int setnonblocking(int fd);

    // 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
    void addfd(int epollfd, int fd, bool one_shot, int TRIGMode);

    // 信号处理函数
    static void sig_handler(int sig);

    // 设置信号函数
    void addsig(int sig, void(handler)(int), bool restart = true);

    // 定时处理任务，重新定时以不断触发SIGALRM信号
    void timer_handler();

    void show_error(int connfd, const char *info);

public:
    static int *u_pipefd;
    sort_timer_heap m_timer_lst;
    static int u_epollfd;
    int m_TIMESLOT;
};

void cb_func(client_data *user_data); // 关闭连接

#endif
