// unifiedEventSource.cpp
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>
#define MAX_EVENT_NUMBER 1024 // 最大事件个数
static int pipefd[2];         // 全局管道
int setnonblocking(int fd)    // 设置非阻塞状态
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
void addfd(int epollfd, int fd)
{
    epoll_event event;                             // 创建事件变量
    event.data.fd = fd;                            // 存储数据fd
    event.events = EPOLLIN | EPOLLET;              // 期待可读事件、边缘触发模式
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event); // 往事件表中注册fd上的事件event
    setnonblocking(fd);                            // 设为非阻塞
}
void sig_handler(int sig) // 信号处理函数
{
    int save_errno = errno; // 保留原来的errno，在函数最后恢复，以保证函数的可重入性
    int msg = sig;
    send(pipefd[1], (char *)&msg, 1, 0); // 将信号值写入管道，以通知主循环
    errno = save_errno;
}
void addsig(int sig) // 设置信号的处理函数
{
    struct sigaction sa;                     // 处理信号的结构体类型
    memset(&sa, '\0', sizeof(sa));           // 初始化
    sa.sa_handler = sig_handler;             // 指定信号处理函数为sig_handler
    sa.sa_flags |= SA_RESTART;               // 设置程序收到信号时的行为，SA_RESTART表示重新调用被信号终止的系统调用
    sigfillset(&sa.sa_mask);                 // sa_mask表示进程的信号掩码，指定哪些信号不能发送给本进程，此语句表示在信号集中设置所有信号，即给mask设定所有信号
    assert(sigaction(sig, &sa, NULL) != -1); // sigaction系统调用，sig是要捕获的信号，sa指定了信号处理方式
}
int main(int argc, char *argv[])
{
    // 处理参数、地址
    if (argc <= 2)
    {
        printf("usage:%s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));
    if (ret == -1)
    {
        printf("errno is%d\n", errno);
        return 1;
    }
    ret = listen(listenfd, 5);
    assert(ret != -1);
    epoll_event events[MAX_EVENT_NUMBER]; // 初始化epoll事件
    int epollfd = epoll_create(5);        // 创建内核事件表
    assert(epollfd != -1);
    addfd(epollfd, listenfd);                          // 添加事件
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd); // 使用socketpair创建双向管道，注册pipefd[0]上的可读事件
    assert(ret != -1);
    setnonblocking(pipefd[1]); // 管道非阻塞
    addfd(epollfd, pipefd[0]); // 添加事件
    // 设置一些信号的处理函数
    addsig(SIGHUP);           // 控制终端挂起
    addsig(SIGCHLD);          // 子进程状态发生变化（退出或暂停）
    addsig(SIGTERM);          // 终止进程，kill 命令默认发送的信号就是 SIGTERM
    addsig(SIGINT);           // 中断信号
    bool stop_server = false; // 关闭服务器标志
    while (!stop_server)      // 主循环
    {
        int number = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1); //-1是timeout，表示永远阻塞
        if ((number < 0) && (errno != EINTR))
        {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < number; i++)
        {
            int sockfd = events[i].data.fd;
            // 如果就绪的文件描述符是listenfd，则处理新的连接
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);
                addfd(epollfd, connfd); // 将本次连接加入内核事件表
            }
            // 如果就绪的文件描述符是pipefd[0]，则处理信号
            else if ((sockfd == pipefd[0]) && (events[i].events & EPOLLIN)) // 来自管道的事件，并且为可读类型
            {
                int sig;
                char signals[1024];
                ret = recv(pipefd[0], signals, sizeof(signals), 0); // 接收数据
                if (ret == -1)
                    continue;
                else if (ret == 0)
                    continue;
                else
                {
                    // 因为每个信号值占1字节，所以按字节来逐个接收信号。我们以SIGTERM为例，来说明如何安全地终止服务器主循环
                    for (int i = 0; i < ret; ++i)
                    {
                        switch (signals[i])
                        {
                        case SIGCHLD:
                        case SIGHUP:
                        {
                            continue;
                        }
                        case SIGTERM:
                        case SIGINT:
                        {
                            stop_server = true; // 设置关闭服务器标志
                        }
                        }
                    }
                }
            }
            else
            {
            }
        }
    }
    printf("close fds\n");
    close(listenfd);
    close(pipefd[1]);
    close(pipefd[0]);
    return 0;
}