#include <functional>
#include <string>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/tcp.h>

#include "Connection.h"
#include "LuckyLog.h"

static EventLoop *CheckLoop(EventLoop *loop){
    if (loop == nullptr){
        LOG_FATAL("%s:%s:%d mainLoop is null!\n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

Connection::Connection(EventLoop *loop,
                    const std::string &nameArg,
                    int sockfd,
                    const Address &localAddr,
                    const Address &peerAddr)
    : loop_(CheckLoop(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64 * 1024 * 1024) {
    channel_->setReadCallback(std::bind(&Connection::handleRead, this, std::placeholders::_1));
    channel_->setWriteCallback(std::bind(&Connection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&Connection::handleClose, this));
    channel_->setErrorCallback(std::bind(&Connection::handleError, this));

    LOG_INFOM("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);

    socket_->setKeepAlive(true);
}

Connection::~Connection(){
    LOG_INFOM("Connection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->getFd(), (int)state_);
}

EventLoop *Connection::getLoop() const{
    return loop_;
}

const std::string &Connection::getName() const{
    return name_;
}

const Address &Connection::localAddress() const{
    return localAddr_;
}

const Address &Connection::peerAddress() const{
    return peerAddr_;
}

bool Connection::is_connect() const{
    return state_ == kConnected;
}


void Connection::send(std::string &buff){
    if (state_ == kConnected){
        if(loop_->isLoopThread()){
            ssize_t nwrote = 0;
            size_t len = buff.size();
            size_t remaining = buff.size();
            bool faultError = false;
            if (state_ == kDisconnected) {
                LOG_ERROR("disconnected, give up writing");
            }
            if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0){
                nwrote = write(channel_->getFd(), buff.c_str(), len);
                if (nwrote >= 0){
                    remaining = len - nwrote;
                    if (remaining == 0 && writeCompleteCallback_){
                        loop_->QueueinLoop(std::bind(writeCompleteCallback_, shared_from_this()));
                    }
                } else {
                    nwrote = 0;
                    if (errno != EWOULDBLOCK) {
                        LOG_ERROR("TcpConnection::sendInLoop");
                        if (errno == EPIPE || errno == ECONNRESET) {
                            faultError = true;
                        }
                    }
                }
            }
            if (!faultError && remaining > 0){
                size_t oldLen = outputBuffer_.readableBytes();
                if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
                {
                    loop_->QueueinLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
                }
                outputBuffer_.append((char *)buff.c_str() + nwrote, remaining);
                if (!channel_->isWriting())
                {
                    channel_->enableWriting(); // 这里一定要注册channel的写事件 否则poller不会给channel通知epollout
                }
            }
        } else {
            //loop_->RuninLoop(std::bind(
                //&Connection::sendInLoop, this, buff.c_str(), buff.size()));
        }
    }

}

void Connection::shutdown(){
    if (state_ == kConnected){
        setState(kDisconnecting);
        loop_->RuninLoop(
            std::bind(&Connection::shutdownInLoop, this));
    }
}

void Connection::shutdownInLoop(){
    if (!channel_->isWriting()) {
        socket_->shutdownWrite();
    }
}


void Connection::setConnectionCallback(const ConnectionCallback &cb){
    connectionCallback_ = cb;
}

void Connection::setMessageCallback(const MessageCallback &cb){
    messageCallback_ = cb;
}

void Connection::setWriteCompleteCallback(const WriteCompleteCallback &cb){
    writeCompleteCallback_ = cb;
}

void Connection::setCloseCallback(const CloseCallback &cb){ 
    closeCallback_ = cb; 
}

void Connection::setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t highWaterMark){ 
    highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; 
}

void Connection::handleRead(TimeStamp receiveTime){
    int savedErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->getFd(), &savedErrno);
    if(n > 0){
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    } else if (n == 0){
        handleClose();
    } else {
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}

void Connection::handleWrite(){
    if (channel_->isWriting()){
        int savedErrno = 0;
        ssize_t n = outputBuffer_.writeFd(channel_->getFd(), &savedErrno);
        if (n > 0)
        {
            outputBuffer_.retrieve(n);
            if (outputBuffer_.readableBytes() == 0){
                channel_->disableWriting();
                if (writeCompleteCallback_){
                    loop_->QueueinLoop(
                            std::bind(writeCompleteCallback_, shared_from_this()));
                }
                if (state_ == kDisconnecting)
                    {
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
            LOG_ERROR("TcpConnection fd=%d is down, no more writing", channel_->getFd());
        }
    }
}

void Connection::handleClose(){
    LOG_INFOM("TcpConnection::handleClose fd=%d state=%d\n", channel_->getFd(), (int)state_);
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this());
    connectionCallback_(connPtr);
    closeCallback_(connPtr);  
}

void Connection::handleError(){
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->getFd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}

void Connection::setState(StateE state){
    state_ = state;
}

void Connection::connectEstablished(){
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading(); 

    connectionCallback_(shared_from_this());
}

void Connection::connectDestroyed(){
    if (state_ == kConnected)
    {
        setState(kDisconnected);
        channel_->disableAll(); 
        connectionCallback_(shared_from_this());
    }
    channel_->RemoveinChannel();
}