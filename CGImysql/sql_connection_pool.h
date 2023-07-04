#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include "../lock/locker.h"
#include "../log/log.h"

using namespace std;

class connection_pool
{
public:
	// 局部静态变量单例模式
	MYSQL *GetConnection();
	// 释放连接
	bool ReleaseConnection(MYSQL *conn);
	// 获取连接
	int GetFreeConn();
	// 销毁所有连接
	void DestroyPool();

	// 单例模式
	static connection_pool *GetInstance();
	// 初始化
	void init(string url, string User, string PassWord, string DataBaseName, int Port, int MaxConn, int close_log);

private:
	// 构造函数
	connection_pool();
	// 析构函数
	~connection_pool();

	int m_MaxConn;			// 最大连接数
	int m_CurConn;			// 当前已使用的连接数
	int m_FreeConn;			// 当前空闲的连接数
	locker lock;			// 互斥锁
	list<MYSQL *> connList; // 连接池
	sem reserve;			// 信号量

public:
	string m_url;		   // 主机地址
	string m_Port;		   // 数据库端口号
	string m_User;		   // 登录数据库用户名
	string m_PassWord;	   // 登录数据库密码
	string m_DatabaseName; // 使用数据库名
	int m_close_log;	   // 日志开关
};

// 资源获取即初始化的连接
class connectionRAII
{
public:
	// 双指针对MYSQL *con修改
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();

private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif
