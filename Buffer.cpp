#include "Buffer.h"

#include <algorithm>
#include <stddef.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>

explicit Buffer::Buffer(size_t initalSize = keyInitialSize)
    : buffer_(keyCheapPrepend + initalSize)
    , readerIndex_(keyCheapPrepend)
    , writerIndex_(keyCheapPrepend)
{

}

size_t Buffer::readableBytes() const { 
    return writerIndex_ - readerIndex_; 
}

size_t Buffer::writableBytes() const { 
    return buffer_.size() - writerIndex_; 
}

size_t Buffer::prependableBytes() const { 
    return readerIndex_; 
}

const char *Buffer::peek() const { 
    return begin() + readerIndex_; 
}

void Buffer::retrieve(size_t len){
    if (len < readableBytes()){
        readerIndex_ += len; 
    }else {
        retrieveAll();
    }
}

void Buffer::retrieveAll(){
    readerIndex_ = keyCheapPrepend;
    writerIndex_ = keyCheapPrepend;
}

std::string Buffer::retrieveAllAsString() { 
    return retrieveAsString(readableBytes()); 
}

std::string Buffer::retrieveAsString(size_t len){
    std::string result(peek(), len);
    retrieve(len);
    return result;
}

void Buffer::ensureWritableBytes(size_t len){
    if (writableBytes() < len){
        makeSpace(len);
    }
}

void Buffer::append(const char *data, size_t len){
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    writerIndex_ += len;
}

char *Buffer::beginWrite() { 
    return begin() + writerIndex_; 
}

const char *Buffer::beginWrite() const { 
    return begin() + writerIndex_; 
}


ssize_t Buffer::readFd(int fd, int *saveErrno){
    char extrabuff[65536] = {0};
    struct iovec vec[2];
    const size_t writable = writableBytes();

    vec[0].iov_base = begin() + writerIndex_;
    vec[0].iov_len = writable;

    vec[1].iov_base = extrabuff;
    vec[1].iov_len = sizeof(extrabuff);

    const int iovcnt = (writable < sizeof(extrabuff)) ? 2 : 1;
    const ssize_t n = readv(fd, vec, iovcnt);

    if(n < 0){
        *saveErrno = errno;
    } else if (n <= writable){
        writerIndex_ += n;
    } else {
        writerIndex_ = buffer_.size();
        append(extrabuff, n - writable);
    }
    return n;
}

ssize_t Buffer::writeFd(int fd, int *saveErrno){
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

char *Buffer::begin() { 
    return &*buffer_.begin(); 
}

const char *Buffer::begin() const { 
    return &*buffer_.begin(); 
}

void Buffer::makeSpace(size_t len){
    if (writableBytes() + prependableBytes() < len + keyCheapPrepend){
        buffer_.resize(writerIndex_ + len);
    } else {
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                begin() + writerIndex_, 
                begin() + keyCheapPrepend);
        readerIndex_ = keyCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}
