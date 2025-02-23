#include "Server.h"

// 检查传入的 baseLoop 指针是否有意义
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

Server::Server(EventLoop *loop,
               const Address &listenAddr,
               const std::string &nameArg,
               Option option)
    : loop_(CheckLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()), name_(nameArg)
      // Aceptor和ManagerPool在这里创建
      ,
      acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)), ManagerPool_(new ManagerPool(loop, name_)), connectionCallback_(), messageCallback_(), nextConnId_(1), started_(0)
{
    // 当有新用户连接时，Acceptor类中绑定的acceptFiled_会有读事件发生，执行handleRead()调用Server::newConnection回调
    acceptor_->setNewConnectionCallback(
        std::bind(&Server::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

Server::~Server()
{
    for (auto &item : connections_)
    {
        ConnectionPtr conn(item.second);
        // 把原始的智能指针复位 让栈空间的ConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
        item.second.reset();
        // 销毁连接
        conn->getLoop()->runinLoop(
            std::bind(&Connection::connectDestroyed, conn));
    }
}

// 设置底层subloop的个数
void Server::setThreadNum(int numThreads)
{
    ManagerPool_->setThread_num(numThreads);
}

// 开启服务器监听
void Server::start()
{
    // 防止一个Server对象被start多次
    if (started_++ == 0)
    {
        ManagerPool_->start(threadInitCallback_);
        // 开启监听
        loop_->runinLoop(std::bind(&Acceptor::listen, acceptor_.get()));
    }
}

// 有一个新用户连接，acceptor会执行这个回调操作
// 负责将mainLoop接收到的请求连接(acceptFiled_会有读事件发生)通过回调轮询分发给subLoop去处理
void Server::newConnection(int sockfd, const Address &peerAddr)
{
    // 轮询算法,选择一个subLoop,来管理connfd对应的filed
    EventLoop *ioLoop = ManagerPool_->getNextLoop();
    char buff[64] = {0};
    snprintf(buff, sizeof(buff), "-%s#%d", ipPort_.c_str(), nextConnId_);
    // 这里没有设置为原子类是因为其只在mainloop中执行,不涉及线程安全问题
    ++nextConnId_;
    // 新连接名称
    std::string connName = name_ + buff;

    LOG_INFOM("TcpServer::newConnection [%s] - new connection [%s] from %s\n", name_.c_str(), connName.c_str(), peerAddr.toIpPort().c_str());

    // 通过sockfd获取其绑定的本机的ip地址和端口信息
    sockaddr_in local;
    memset(&local, 0, sizeof(local));
    socklen_t addrlen = sizeof(local);

    if (::getsockname(sockfd, (sockaddr *)&local, &addrlen) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    // 用本地ip端口创建Address对象
    Address localAddr(local);
    ConnectionPtr conn(new Connection(ioLoop, connName, sockfd, localAddr, peerAddr));
    // 放入connected_map中
    connections_[connName] = conn;
    // 下面的回调都是用户设置给Server => Connection的，至于Filed绑定的则是Connection设置的四个，handleRead,handleWrite... 这下面的回调用于handlexxx函数中
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 设置了如何关闭连接的回调
    conn->setCloseCallback(
        std::bind(&Server::removeConnection, this, std::placeholders::_1));
    // 执行回调函数
    ioLoop->runinLoop(
        std::bind(&Connection::connectEstablished, conn));
}

// 移除连接
void Server::removeConnection(const ConnectionPtr &conn)
{
    loop_->runinLoop(
        std::bind(&Server::removeConnectionInLoop, this, conn));
}

void Server::removeConnectionInLoop(const ConnectionPtr &conn)
{
    LOG_INFOM("TcpServer::removeConnectionInLoop [%s] - connection %s\n", name_.c_str(), conn->getName().c_str());

    connections_.erase(conn->getName());
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueinLoop(
        std::bind(&Connection::connectDestroyed, conn));
}

void Server::setThreadInitCallback(const ThreadInitCallback &cb)
{
    threadInitCallback_ = cb;
}

void Server::setConnectionCallback(const ConnectionCallback &cb)
{
    connectionCallback_ = cb;
}

void Server::setMessageCallback(const MessageCallback &cb)
{
    messageCallback_ = cb;
}

void Server::setWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    writeCompleteCallback_ = cb;
}