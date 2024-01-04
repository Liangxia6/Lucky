#pragma once 

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <cstring>

#include "LuckyLog.h"
#include "EventLoop.h"
#include "Acceptor.h"
#include "Address.h"
#include "ThreadPool.h"
#include "Connection.h"
#include "Manager.h"
#include "Buffer.h"

/**
 * 
*/
class Server
{
public:
    //定义callback
    using ConnectionPtr = std::shared_ptr<Connection>;
    using ConnectionCallback = std::function<void(const ConnectionPtr &)>;
    using WriteCompleteCallback = std::function<void(const ConnectionPtr &)>;
    using MessageCallback = std::function<void(const ConnectionPtr &, Buffer *, TimeStamp)>;
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    //端口复用
    enum Option
    {
        kNoReusePort,
        kReusePort,
    };

    Server(EventLoop *, const Address &, const std::string &, Option  = kNoReusePort);
    ~Server();

    //设置回调函数
    void setThreadInitCallback(const ThreadInitCallback &);
    void setConnectionCallback(const ConnectionCallback &);
    void setMessageCallback(const MessageCallback &);
    void setWriteCompleteCallback(const WriteCompleteCallback &);

    //设置开启的子线程数量
    void setThreadNum(int );

    //启动服务器
    void start();

private:

    //处理连接的函数群
    void newConnection(int, const Address &);
    void removeConnection(const ConnectionPtr &);
    void removeConnectionInLoop(const ConnectionPtr &);

    EventLoop *loop_;               //main中开启的主EventLoop

    const std::string ipPort_;      //传入的ip端口
    const std::string name_;        //Server的名称

    std::atomic_int started_;   //指示符
    int nextConnId_;            //指向下一链接的id

    std::unique_ptr<Acceptor> acceptor_;        //接收器,用于接收新连接
    std::shared_ptr<ThreadPool> threadPool_;    //用于管理子线程

    ConnectionCallback connectionCallback_;      
    MessageCallback messageCallback_;            
    WriteCompleteCallback writeCompleteCallback_;
    ThreadInitCallback threadInitCallback_;     //EventLoop线程初始化的回调函数

    using ConnectionMap = std::unordered_map<std::string, ConnectionPtr>;
    ConnectionMap connections_;                 //保存所有连接
};