#include <string>
#include <iostream>
#include <Lucky/Server.h>
#include <Lucky/LuckyLog.h>

class EchoServer
{
public:
    EchoServer(EventLoop *loop, const Address &addr, const std::string &name)
        : server_(loop, addr, name)
        , loop_(loop)
    {
        // 注册回调函数
        server_.setConnectionCallback(
            std::bind(&EchoServer::onConnection, this, std::placeholders::_1));
        
        server_.setMessageCallback(
            std::bind(&EchoServer::onMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        // 设置合适的subloop线程数量
        server_.setThreadNum(3);
    }
    void start()
    {
        server_.start();
    }

private:
    // 连接建立或断开的回调函数
    void onConnection(const std::shared_ptr<Connection> &conn)   
    {
        if (conn->isConnect())
        {
            std::cout<<"111"<<std::endl;
            log<INFOM>("Connection UP : %s", conn->getPeerAddress().toIpPort().c_str());
        }
        else
        {
            log<INFOM>("Connection DOWN : %s", conn->getPeerAddress().toIpPort().c_str());
        }
    }

    // 可读写事件回调
    void onMessage(const std::shared_ptr<Connection> &conn, Buffer *buf, TimeStamp time)
    {
        std::cout<<"222"<<std::endl;
        std::string msg = buf->retrieveAllAsString();
        conn->send(msg);
        // conn->shutdown();   // 关闭写端 底层响应EPOLLHUP => 执行closeCallback_
    }

    EventLoop *loop_;
    Server server_;
};

int main() {
    EventLoop loop;
    Address addr(8002);
    EchoServer server(&loop, addr, "EchoServer");
    server.start();
    loop.loop();
    return 0;
}