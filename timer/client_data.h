#ifndef CLIENT
#define CLIENT
#include <netinet/in.h>
#include "../timer/lst_timer.h"
#include "../timer/heap_timer.h"

class util_lst_timer;
class util_heap_timer;

// 用户数据
struct client_data
{
    // 客户端socket地址
    sockaddr_in address;

    // socket文件描述符
    int sockfd;

    // 定时器
    util_heap_timer *heap_timer;
    util_lst_timer *lst_timer;
};

#endif