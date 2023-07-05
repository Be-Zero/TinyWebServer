#ifndef HEAP_TIMER
#define HEAP_TIMER

#include <time.h>
#include <vector>
#include <algorithm>
#include "../timer/client_data.h"
using namespace std;

struct client_data;

// 连接资源结构体成员需要用到定时器类
// 计时器
class util_heap_timer
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
    // 添加定时器，内部调用私有成员add_timer
    void add_timer(util_heap_timer *timer);

    // 调整定时器，任务发生变化时，调整定时器在链表中的位置
    void adjust_timer(util_heap_timer *timer);

    // 删除计时器
    void del_timer(util_heap_timer *timer);

    // 计时，并处理到期的计时器
    void tick();

private:
    // 定义比较器
    struct cmp
    {
        bool operator()(util_heap_timer *a, util_heap_timer *b) const
        {
            return a->expire > b->expire;
        }
    };

    // 存储计时器的动态数组
    vector<util_heap_timer *> heap;
};

#endif
