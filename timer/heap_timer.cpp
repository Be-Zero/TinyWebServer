#include "heap_timer.h"
#include "../http/http_conn.h"

// 添加定时器，内部调用私有成员add_timer
void sort_timer_heap::add_timer(util_timer *timer)
{
    if (!timer)
    {
        return;
    }
    heap.push_back(timer);
    make_heap(heap.begin(), heap.end(), cmp());
}

// 调整定时器，任务发生变化时，调整定时器在链表中的位
void sort_timer_heap::adjust_timer(util_timer *timer)
{
    make_heap(heap.begin(), heap.end(), cmp());
}

// 删除计时器
void sort_timer_heap::del_timer(util_timer *timer)
{
    for (auto it = heap.begin(); it != heap.end(); ++it)
    {
        if (*it == timer)
        {
            heap.erase(it);
            make_heap(heap.begin(), heap.end(), cmp());
            break;
        }
    }
}

// 计时，并处理到期的计时器
void sort_timer_heap::tick()
{
    if (heap.empty())
        return;
    time_t cur = time(NULL);
    int ct = 0;
    for (; ct < heap.size(); ++ct)
    {
        if (cur < heap[ct]->expire)
            break;
        heap[ct]->cb_func(heap[ct]->user_data);
        delete heap[ct];
    }
    for (int i = 0; i < ct; ++i)
    {
        heap[0] = heap.back();
        heap.pop_back();
    }
    make_heap(heap.begin(), heap.end(), cmp());
}

// 设置最小超时单位
void Utils::init(int timeslot)
{
    m_TIMESLOT = timeslot;
}

// 对文件描述符设置非阻塞
int Utils::setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
void Utils::addfd(int epollfd, int fd, bool one_shot, int TRIGMode)
{
    epoll_event event;
    event.data.fd = fd;

    // 期待可读、关闭连接，采用ET工作模式
    if (1 == TRIGMode)
        event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    else
        event.events = EPOLLIN | EPOLLRDHUP;

    // EPOLLONESHOT模式能够保证同一fd只有一个线程处理来处理，不会触发竞态
    if (one_shot)
        event.events |= EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

// 信号处理函数
void Utils::sig_handler(int sig)
{
    // 为保证函数的可重入性，保留原来的errno
    // 可重入性表示中断后再次进入该函数，环境变量与之前相同，不会丢失数据
    int save_errno = errno;
    int msg = sig;
    // 将信号值从管道写端写入，传输字符类型，而非整型
    send(u_pipefd[1], (char *)&msg, 1, 0);
    // 将原来的errno赋值为当前的errno
    errno = save_errno;
}

// 设置信号函数
void Utils::addsig(int sig, void(handler)(int), bool restart)
{
    // 创建sigaction结构体变量
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));

    // 信号处理函数中仅仅发送信号值，不做对应逻辑处理
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;

    // 将所有信号添加到信号集中
    sigfillset(&sa.sa_mask);

    // 执行sigaction函数
    assert(sigaction(sig, &sa, NULL) != -1);
}

// 定时处理任务，重新定时以不断触发SIGALRM信号
void Utils::timer_handler()
{
    m_timer_heap.tick();
    alarm(m_TIMESLOT);
}

// 发送错误信息
void Utils::show_error(int connfd, const char *info)
{
    send(connfd, info, strlen(info), 0);
    close(connfd);
}

int *Utils::u_pipefd = 0;
int Utils::u_epollfd = 0;

class Utils;

// 定时器回调函数
void cb_func(client_data *user_data)
{
    // 删除非活动连接在socket上的注册事件
    epoll_ctl(Utils::u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);

    // 关闭文件描述符
    close(user_data->sockfd);

    // 减少连接数
    http_conn::m_user_count--;
}
