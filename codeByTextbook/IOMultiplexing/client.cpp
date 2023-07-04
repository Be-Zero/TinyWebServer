#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>
#define BUFFER_SIZE 64
int main(int argc, char *argv[])
{
    if (argc <= 2)  // 判断参数个数，第一个为ip地址，第二个为端口号
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }
    const char *ip = argv[1];  // 存储字符串ip地址
    int port = atoi(argv[2]);  // 存储int型端口号
    struct sockaddr_in server_address;  // 定义sockaddr_in地址结构体
    bzero(&server_address, sizeof(server_address));  // 将server_address内存块的值清零
    server_address.sin_family = AF_INET;  // 设置地址族为AF_INET
    inet_pton(AF_INET, ip, &server_address.sin_addr);  // 字符串ip地址转为网络字节序
    server_address.sin_port = htons(port);  // 设置端口号，htons将端口号转为网络字节序
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);  // 创建socket，PF_INET为ipv4协议族，SOCK_STREAM表示传输层使用tcp协议，第三个参数表示具体协议，通常为0
    assert(sockfd >= 0);  // 验证创建是否成功
    if (connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)  // connect发起连接，将sockaddr_in类型强制转换为sockaddr*，第三个参数是地址长度，此时判断连接是否成功
    {
        printf("connection failed\n");  // 连接失败
        close(sockfd);  // 关闭socket
        return 1;
    }
    pollfd fds[2];  // 使用poll系统调用
    /*注册文件描述符0（标准输入）和文件描述符sockfd上的可读事件*/
    fds[0].fd = 0;  // 文件描述符，0表示标准输入
    fds[0].events = POLLIN;  // 告诉poll监听fd上的哪些事件，POLLIN为数据可读
    fds[0].revents = 0;  // 实际发生的事件，由内核修改，以通知应用程序fd上实际发生了什么
    fds[1].fd = sockfd;  // 使得fds[1]为sockfd
    fds[1].events = POLLIN | POLLRDHUP;  // 数据可读、TCP连接被对方关闭、对方关闭写操作
    fds[1].revents = 0;
    char read_buf[BUFFER_SIZE];  // 读缓存区
    int pipefd[2];  // 存储管道数据
    int ret = pipe(pipefd);  // 创建管道，pipefd[0]为读出端，pipefd[1]为写入端
    assert(ret != -1);  // 判断管道是否创建成功
    while (1)  // 循环处理
    {
        ret = poll(fds, 2, -1);  // 轮询文件描述符以测试其中是否有就绪者，第二个参数表示被监听事件集合fds的大小，第三个参数为超时时间，-1表示永远阻塞直到某个事件发生
        if (ret < 0)  // 判断返回，成功时返回就绪的文件描述符总数，失败时为-1
        {
            printf("poll failure\n");
            break;
        }
        if (fds[1].revents & POLLRDHUP)  // 判断fds[1]的类型是否为POLLRDHUP，即被关闭TCP或关闭写操作
        {
            printf("server close the connection\n");
            break;
        }
        else if (fds[1].revents & POLLIN)  // 判断fds[1]是否可读，即获取服务器发送来的数据
        {
            memset(read_buf, '\0', BUFFER_SIZE);  // 初始化读缓存
            recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);  // 读取TCP数据，第三个参数为读缓冲区的大小，flags为数据收发的额外控制器，0表示无意义
            printf("%s\n", read_buf);  // 输出读缓存区的数据
        }
        if (fds[0].revents & POLLIN)  // 判断fds[0]是否可读，即处理用户输入的数据
        { 
            // 使用splice将用户输入的数据直接写到sockfd上（零拷贝），第一个参数为待输出数据的文件描述符，第二个参数设定数据流从何处开始读取（NULL表示从当前偏移位置读取）
            // 第三个参数为输出数据流的文件描述符，第四个参数同上，第五个参数为移动数据的长度，第六个参数表示控制数据如何移动
            // 0表示标准输入，写入到管道pipefd[1]，数据长度为32768，整页移动|后续将读取更多数据
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
            // pipefd[0]表示从管道中读出，发送给sockfd，数据长度为32768，整页移动|后续将读取更多数据
            ret = splice(pipefd[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(sockfd);  // 关闭连接
    return 0;
}