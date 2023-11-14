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
    , state_(keyConnecting)
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
    return state_ == keyConnected;
}


void Connection::send(std::string &buff){
    if (state_ == keyConnected){
        if(loop_->isLoopThread()){
            ssize_t nwrote = 0;
            size_t len = buff.size();
            size_t remaining = buff.size();
            bool faultError = false;
            if (state_ == keyDisconnected) {
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
            loop_->RuninLoop(std::bind(
                &Connection::sendInLoop, this, buff.c_str(), buff.size()));
        }
    }

}

void Connection::shutdown(){
    if (state_ == keyConnected){
        setState(keyDisconnecting);
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

}

    void handleWrite();
    void handleClose();
    void handleError();