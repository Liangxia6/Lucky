#include "Connection.h"

static EventLoop *CheckLoop(EventLoop *loop)
{
    if (loop == nullptr)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

Connection::Connection(EventLoop *loop,
                       const std::string &name,
                       int sockfd,
                       const Address &localAddr,
                       const Address &peerAddr)
    : loop_(CheckLoop(loop)), name_(name), state_(kConnecting), reading_(true), socket_(new Socket(sockfd)), filed_(new Filed(loop, sockfd)), localAddr_(localAddr), peerAddr_(peerAddr), highWaterMark_(64 * 1024 * 1024) // 64M
{
    filed_->setReadCallback(std::bind(&Connection::handleRead, this, std::placeholders::_1));
    filed_->setWriteCallback(std::bind(&Connection::handleWrite, this));
    filed_->setCloseCallback(std::bind(&Connection::handleClose, this));
    filed_->setErrorCallback(std::bind(&Connection::handleError, this));

    LOG_INFOM("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}

Connection::~Connection()
{
    LOG_INFOM("Connection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), filed_->getFd(), (int)state_);
}

void Connection::send(std::string &buff)
{
    if (state_ == kConnected)
    {
        //(用于单个reactor)如果所在线程是自己绑定的EventLoop线程,直接发送
        if (loop_->isLoopThread())
        {
            sendinLoop(buff.c_str(), buff.size());
        }
        else
        {
            loop_->runinLoop(std::bind(&Connection::sendinLoop, this, buff.c_str(), buff.size()));
        }
    }
}

void Connection::sendinLoop(const void *data, size_t len)
{
    ssize_t nwrote = 0;      // 已经发送的数据长度
    size_t remaining = len;  // 还剩下多少数据需要发送
    bool faultError = false; // 记录是否产生错误

    // 已经调用过shundown,关闭连接不再发送
    if (state_ == kDisconnected)
    {
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 刚初始化的filed和数据发送完毕的filed都是没有再epoll注册可写时间
    // 且outputBuffer_中空了
    if (!filed_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        nwrote = write(filed_->getFd(), data, len);
        if (nwrote > 0)
        {
            remaining = len - nwrote;
            if (remaining == 0 && writeCompleteCallback_)
            {
                // 既然在这里数据全部发送完成，就不用再给filed设置epollout事件了
                loop_->queueinLoop(
                    std::bind(writeCompleteCallback_, shared_from_this()));
            }
            else
            {
                nwrote = 0;
                // EWOULDBLOCK表示非阻塞情况下没有数据后的正常返回 等同于EAGAIN
                if (errno != EWOULDBLOCK)
                {
                    faultError = true;
                }
            }
        }
    }
    /**
     * 说明当前这一次write并没有把数据全部发送出去 剩余的数据需要保存到缓冲区当中
     * 然后给filed注册EPOLLOUT事件，Epoller发现tcp的发送缓冲区有空间后会通知
     * 相应的sock->filed，调用filed对应注册的writeCallback_回调方法，
     * filed的writeCallback_实际上就是TcpConnection设置的handleWrite回调，
     * 把发送缓冲区outputBuffer_的内容全部发送完成
     **/
    if (!faultError && remaining > 0)
    {
        // 目前发送缓冲区剩余的待发送的数据的长度
        size_t oldLen = outputBuffer_.readableBytes();
        // 判断待写数据是否会超过设置的高位标志highWaterMark_
        if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            loop_->queueinLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }
        // 将data中剩余还没有发送的数据最佳到buffer中
        outputBuffer_.append((char *)data + nwrote, remaining);
        if (!filed_->isWriting())
        {
            // 这里一定要注册filed的写事件 否则Epoller不会给filed通知epollout
            filed_->enableWriting();
        }
    }
}

// 关闭连接
void Connection::shutdown()
{
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        loop_->runinLoop(
            std::bind(&Connection::shutdownInLoop, this));
    }
}

void Connection::shutdownInLoop()
{
    // 说明当前outputBuffer_的数据全部向外发送完成
    if (!filed_->isWriting())
    {
        socket_->shutdownWrite();
    }
}

// 读是相对服务器而言的 当对端客户端有数据到达 服务器端检测到EPOLLIN 就会触发该fd上的回调 handleRead取读走对端发来的数据
void Connection::handleRead(TimeStamp receiveTime)
{
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(filed_->getFd(), &savedErrno);
    if (n > 0)
    {
        // 已建立连接的用户有可读事件发生了 调用用户传入的回调操作onMessage, shared_from_this就是获取了TcpConnection的智能指针
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }
    else if (n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void Connection::handleWrite()
{
    if (filed_->isWriting())
    {
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(filed_->getFd(), &savedErrno);
        if (n > 0)
        {
            // 把outputBuffer_的readerIndex往前移动n个字节, 因为outputBuffer_中readableBytes已经发送出去了n字节
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0)
            {
                // 数据发送完毕后注销写事件，以免epoll频繁触发可写事件，导致效力低下
                filed_->disableWriting();
                if (writeCompleteCallback_)
                {
                    // Connection对象在其所在的subloop中, 向pendFunctors_中加入回调
                    loop_->queueinLoop(
                        std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                {
                    // 在当前所属的loop中把TcpConnection删除掉
                    shutdownInLoop();
                }
            }
            else
            {
                LOG_ERROR("TcpConnection::handleWrite");
            }
        }
        else
        {
            LOG_ERROR("TcpConnection fd=%d is down, no more writing", filed_->getFd());
        }
    }
}

void Connection::handleClose()
{
    LOG_INFOM("TcpConnection::handleClose fd=%d state=%d\n", filed_->getFd(), (int)state_);
    setState(kDisconnected);
    filed_->disableAll();

    ConnectionPtr connPtr(shared_from_this());
    // 执行连接关闭的回调(用户自定的，而且和新连接到来时执行的是同一个回调)
    connectionCallback_(connPtr);
    // 执行关闭连接的回调 执行的是Server::removeConnection回调方法
    closeCallback_(connPtr);
}

void Connection::handleError()
{
    int optval;
    socklen_t optlen = sizeof(optval);
    int err = 0;
    // 获取并清除socket错误状态,getsockopt成功则返回0,失败则返回-1
    // int getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
    // level：表示选项所属的协议层。常见的值包括 SOL_SOCKET（通用套接字选项）、IPPROTO_TCP（TCP 协议选项）和 IPPROTO_IP（IP 协议选项）等。
    // optname：表示具体的选项名，如 SO_REUSEADDR（允许地址重用）、TCP_NODELAY（禁用 Nagle 算法）等。
    // optval：指向存储选项值的缓冲区的指针。
    // optlen：指向存储 optval 缓冲区大小的变量的指针。在调用函数时，需要将其设置为 optval 缓冲区的大小，函数返回时该变量将被设置为实际的选项值的大小。
    if (getsockopt(filed_->getFd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}

void Connection::setState(StateE state)
{
    state_ = state;
}

// 连接已建立
void Connection::connectEstablished()
{
    setState(kConnected);
    filed_->tie(shared_from_this());
    filed_->enableReading();

    connectionCallback_(shared_from_this());
}

// 连接已销毁
void Connection::connectDestroyed()
{
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        filed_->disableAll();
        connectionCallback_(shared_from_this());
    }
    filed_->remove();
}

EventLoop *Connection::getLoop() const
{
    return loop_;
}

const std::string &Connection::getName() const
{
    return name_;
}

const Address &Connection::getLocalAddress() const
{
    return localAddr_;
}

const Address &Connection::getPeerAddress() const
{
    return peerAddr_;
}

bool Connection::isConnect() const
{
    return state_ == kConnected;
}

void Connection::setConnectionCallback(const ConnectionCallback &cb)
{
    connectionCallback_ = cb;
}

void Connection::setMessageCallback(const MessageCallback &cb)
{
    messageCallback_ = cb;
}

void Connection::setWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    writeCompleteCallback_ = cb;
}

void Connection::setCloseCallback(const CloseCallback &cb)
{
    closeCallback_ = cb;
}

void Connection::setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark)
{
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
}