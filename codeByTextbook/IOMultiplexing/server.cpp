#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <poll.h>
#define USER_LIMIT 5  // 最大用户数量
#define BUFFER_SIZE 64  // 读缓冲区的大小
#define FD_LIMIT 10  // 文件描述符数量限制

struct client_data  // 客户数据
{
    sockaddr_in address;  // socket地址
    char *write_buf;  // 待写入客户端的数据的位置
    char buf[BUFFER_SIZE];  // 从客户端读入的数据
};

inline int setnonblocking(int fd)  // 设定非阻塞的fd
{
    int old_option = fcntl(fd, F_GETFL);  // 提供对文件描述符的各种控制操作，F_GETFL表示获取fd的状态标志
    int new_option = old_option | O_NONBLOCK;  // 表示设置非阻塞标志
    fcntl(fd, F_SETFL, new_option);  // 设置fd的状态标志为new_option
    return old_option;  // 返回旧状态以便日后恢复使用
}

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
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
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);  // 创建socket，ipv4协议族，使用tcp协议
    assert(listenfd >= 0);  // 判断创建是否成功
    ret = bind(listenfd, (struct sockaddr *)&address, sizeof(address));  // 将address所指的socket地址分配给未命名的sockfd文件描述符，第三个参数指出地址的长度
    assert(ret != -1);  // 绑定是否成功
    ret = listen(listenfd, 5);  // 监听socket，内核监听队列的最大长度为5
    assert(ret != -1);  // 判断是否监听成功
    /*创建users数组，分配FD_LIMIT个client_data对象。可以预期：
    每个可能的socket连接都可以获得一个这样的对象，并且socket的值可以直接用来索引（作为数组的下标）socket连接对应的client_data对象，
    这是将socket和客户数据关联的简单而高效的方式*/
    client_data *users = new client_data[FD_LIMIT];  // 存有地址、写缓存、读缓存
    /*尽管我们分配了足够多的client_data对象，但为了提高poll的性能，仍然有必要限制用户的数量*/
    pollfd fds[USER_LIMIT + 1];  // 多1个，因为fds[0]要处理服务
    int user_counter = 0;
    for (int i = 1; i <= USER_LIMIT; ++i)  // 初始化USER_LIMIT个fds
    {
        fds[i].fd = -1;  // 文件描述符
        fds[i].events = 0;  // 期待的事件
    }
    fds[0].fd = listenfd;  // 0号fds设定为监听
    fds[0].events = POLLIN | POLLERR;  // 期待数据可读和错误发生
    fds[0].revents = 0;
    while (1)  // 循环处理
    {
        ret = poll(fds, user_counter + 1, -1);  // poll调用，被监听事件集合的大小为user_counter+1，阻塞等待
        if (ret < 0)  // poll失败
        {
            printf("poll failure\n");
            break;
        }
        for (int i = 0; i < user_counter + 1; ++i)  // 出线预期事件，循环判断所有用户
        {
            if ((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))  // 如果fds为监听者且接收到的事件为数据可读
            {
                struct sockaddr_in client_address;  // 创建客户地址
                socklen_t client_addrlength = sizeof(client_address);  // 初始化
                int connfd = accept(listenfd, (struct sockaddr *)&client_address, &client_addrlength);  // 接收连接，监听fd，源地址，地址长度
                if (connfd < 0)  // 接收失败
                {
                    printf("errno is: %d\n", errno);
                    continue;
                } 
                if (user_counter >= USER_LIMIT)  // 用户请求太多，关闭新到的连接
                {
                    const char *info = "too many users\n";
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);  // 向connfd写数据
                    close(connfd);
                    continue;
                }

                // 对于新的连接，同时修改fds和users数组。前文已经提到，users[connfd]对应 于新连接文件描述符connfd的客户数据
                user_counter++;  // 记录连接的新用户
                users[connfd].address = client_address;  // 设定地址
                setnonblocking(connfd);  // 设定非阻塞的fd
                fds[user_counter].fd = connfd;  // 设定具体的fd
                fds[user_counter].events = POLLIN | POLLRDHUP | POLLERR;  // 期待事件：可读、关闭、错误
                fds[user_counter].revents = 0;
                printf("comes a new user, now have %d users\n", user_counter);
            }
            else if (fds[i].revents & POLLERR)  // 响应错误事件
            {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t length = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0)  // 读取属性：fd，ipv4协议，获取并清除错误状态，被操作选项的值，长度
                    printf("get socket option failed\n");  // 获取失败
                continue;
            }
            else if (fds[i].revents & POLLRDHUP)  // 客户端关闭连接
            {
                users[fds[i].fd] = users[fds[user_counter].fd];  // 将最后一个用户的fd放到i的缓存区位置
                close(fds[i].fd);  // 关闭连接
                fds[i] = fds[user_counter];  // 修改fds
                i--;  // 处理i，否则会忽略最后一个用户
                user_counter--;  // 处理用户个数
                printf("a client left\n");
            }
            else if (fds[i].revents & POLLIN)  // 数据可读
            {
                int connfd = fds[i].fd;  // 设定fd
                memset(users[connfd].buf, '\0', BUFFER_SIZE);  // 初始化用户数据空间
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);  // 接收数据，得到数据长度
                if(memcmp(users[connfd].buf, "exit", 4) == 0) {  // 如果客户端输入exit就关闭服务器
                    delete[] users;  // 删除所有用户
                    close(listenfd);  // 关闭监听
                    return 0;
                }
                printf("get %d bytes of client data: %s from %d\n", ret, users[connfd].buf, connfd);  // 输出信息和发言用户id
                if (ret < 0)  // 错误
                {
                    if (errno != EAGAIN)  // EAGAIN表示再次尝试
                    {
                        close(connfd);  // 关闭连接
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                }
                else if (ret == 0)  // 表示对方关闭连接，暂不处理
                {
                }
                else  // 数据接收正常，通知其他socket写数据给其他用户
                {
                    for (int j = 1; j <= user_counter; ++j)  // 逐用户写数据
                    {
                        if (fds[j].fd == connfd)  // 不需要给发言人写数据
                        {
                            continue;
                        }
                        fds[j].events |= ~POLLIN;  // 期待事件非读
                        fds[j].events |= POLLOUT;  // 期待事件为写
                        users[fds[j].fd].write_buf = users[connfd].buf;  // 指针指向其他用户的读缓存区
                    }
                }
            }
            else if (fds[i].revents & POLLOUT)  // 可写数据
            {
                int connfd = fds[i].fd;  // 处理需要写的dfs
                if (!users[connfd].write_buf)  // 写缓存为空就跳过
                {
                    continue;
                }
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);  // 调用send发送数据
                users[connfd].write_buf = NULL;  // 写完数据后需要重新注册fds[i]上的可读事件
                fds[i].events |= ~POLLOUT;  // 期待事件非写
                fds[i].events |= POLLIN;  // 期待事件为读
            }
        }
    }
    delete[] users;  // 删除所有用户
    close(listenfd);  // 关闭监听
    return 0;
}