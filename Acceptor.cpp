#include "Acceptor.h"

// 创捷一个非阻塞的fd
static int createNonblockFd()
{
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop, const Address &listenAddr, bool reuseport)
    : is_listen_(false), acceptSocket_(createNonblockFd()), acceptFiled_(loop, acceptSocket_.getFd()), loop_(loop)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    // Server::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调
    //(accept => connfd => 打包成Filed => 唤醒subloop)
    // baseloop监听到有事件发生 => acceptFiled_(listenfd) => 执行该回调函数
    acceptSocket_.bindAddress(listenAddr);
    // 为acceptFiled_的fd绑定可读事件，当有新连接到来时，acceptFiled_的fd可读
    acceptFiled_.setReadCallback(
        std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
    // 把从Poller中感兴趣的事件删除掉
    acceptFiled_.disableAll();
    // 调用EventLoop->remove => EPoller->remove 把EPoller的FiledMap对应的部分删除
    acceptFiled_.remove();
}

void Acceptor::setNewConnectionCallback(const NewConnectionCallback &NewConnectionCallback)
{
    newConnectionCallback_ = NewConnectionCallback;
}

bool Acceptor::isListen() const
{
    return is_listen_;
}

void Acceptor::listen()
{
    is_listen_ = true;
    acceptSocket_.listen();
    // 将acceptFiled的读事件注册到poller
    acceptFiled_.enableReading();
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead()
{
    Address peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0)
    {
        if (newConnectionCallback_)
        {
            // 分发任务到子loop
            newConnectionCallback_(connfd, peerAddr);
        }
        else
        {
            LOG_DEBUG("no newConnectionCallback()");
            close(connfd);
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        // 当前进程的fd已经用完了
        // 可以调整单个服务器的fd上限
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}
