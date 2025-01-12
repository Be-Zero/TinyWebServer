#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>

#include "./threadpool/threadpool.h"
#include "./http/http_conn.h"

const int MAX_FD = 65536;           // 最大文件描述符
const int MAX_EVENT_NUMBER = 10000; // 最大事件数
const int TIMESLOT = 5;             // 最小超时单位

class WebServer
{
public:
    WebServer();  // 构造函数
    ~WebServer(); // 析构函数
    // 初始化
    void init(int port, string user, string passWord, string databaseName,
              int log_write, int opt_linger, int trigmode, int sql_num,
              int thread_num, int close_log, int actor_model, int timer_model);

    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void eventListen();
    void eventLoop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(util_heap_timer *timer);
    void adjust_timer(util_lst_timer *timer);
    void deal_timer(util_heap_timer *timer, int sockfd);
    void deal_timer(util_lst_timer *timer, int sockfd);
    bool dealclinetdata();
    bool dealwithsignal(bool &timeout, bool &stop_server);
    void dealwithread(int sockfd);
    void dealwithwrite(int sockfd);

public:
    // 基础
    int m_port;       // 端口
    char *m_root;     // 根目录
    int m_log_write;  // log写入方式
    int m_close_log;  // 关闭log
    int m_actormodel; // 反应堆模式

    int m_pipefd[2];  // 管道
    int m_epollfd;    // epoll系统调用fd
    http_conn *users; // http_conn类对象，记录连接用户的信息

    // 数据库相关
    connection_pool *m_connPool; // 数据库连接池
    string m_user;               // 登陆数据库用户名
    string m_passWord;           // 登陆数据库密码
    string m_databaseName;       // 使用数据库名
    int m_sql_num;               // 连接个数

    // 线程池相关
    threadpool<http_conn> *m_pool;
    int m_thread_num;

    // epoll_event相关
    epoll_event events[MAX_EVENT_NUMBER];

    int m_listenfd;       // 监听fd
    int m_OPT_LINGER;     // 优雅关闭
    int m_TRIGMode;       // 触发模式
    int m_LISTENTrigmode; // 监听事件的触发模式
    int m_CONNTrigmode;   // tcp连接的触发模式

    // 定时器相关
    int m_timer_model;
    client_data *users_timer;
    Utils utils ;
};
#endif
