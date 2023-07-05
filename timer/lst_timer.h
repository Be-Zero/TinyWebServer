#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>
#include "../timer/client_data.h"

struct client_data;

// 连接资源结构体成员需要用到定时器类
// 计时器
class util_lst_timer
{
public:
    util_lst_timer() : prev(NULL), next(NULL) {}

public:
    // 超时时间
    time_t expire;

    // 回调函数
    void (*cb_func)(client_data *);

    // 连接资源
    client_data *user_data;

    // 前向定时器
    util_lst_timer *prev;

    // 后继定时器
    util_lst_timer *next;
};

// 升序计时器
class sort_timer_lst
{
public:
    sort_timer_lst();  // 初始化链表
    ~sort_timer_lst(); // 回收链表空间

    // 添加定时器，内部调用私有成员add_timer
    void add_timer(util_lst_timer *timer);

    // 调整定时器，任务发生变化时，调整定时器在链表中的位置
    void adjust_timer(util_lst_timer *timer);

    // 删除计时器
    void del_timer(util_lst_timer *timer);

    // 计时，并处理到期的计时器
    void tick();

private:
    // 私有成员，被公有成员add_timer和adjust_time调用
    // 主要用于调整链表内部结点
    void add_timer(util_lst_timer *timer, util_lst_timer *lst_head);
    // 头尾结点
    util_lst_timer *head;
    util_lst_timer *tail;
};

#endif