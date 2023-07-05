#include "../timer/heap_timer.h"
#include "../http/http_conn.h"

// 添加定时器，内部调用私有成员add_timer
void sort_timer_heap::add_timer(util_heap_timer *timer)
{
    if (!timer)
    {
        return;
    }
    heap.push_back(timer);
    push_heap(heap.begin(), heap.end());
}

// 调整定时器，任务发生变化时，调整定时器在链表中的位
void sort_timer_heap::adjust_timer(util_heap_timer *timer)
{
    make_heap(heap.begin(), heap.end(), cmp());
}

// 删除计时器
void sort_timer_heap::del_timer(util_heap_timer *timer)
{
    auto it = find(heap.begin(), heap.end(), timer);
    if (it != heap.end())
    {
        iter_swap(it, heap.end() - 1);
        delete heap.back();
        heap.pop_back();
        make_heap(heap.begin(), heap.end(), cmp());
    }
}

// 计时，并处理到期的计时器
void sort_timer_heap::tick()
{
    if (heap.empty())
        return;
    time_t cur = time(NULL);
    int ct = 0;
    while (cur >= heap.front()->expire)
    {
        pop_heap(heap.begin(), heap.end());
        delete heap.back();
        heap.pop_back();
    }
}
