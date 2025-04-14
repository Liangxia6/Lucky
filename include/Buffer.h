#pragma once

#include <algorithm>
#include <stddef.h>
#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
#include <vector>
#include <string>


/*
Buffer的样子
+-------------------------+----------------------+---------------------+
|    prependable bytes    |    readable bytes    |    writable bytes   |
|                         |      (CONTENT)       |                     |
+-------------------------+----------------------+---------------------+
|                         |                      |                     |
0        <=           readerIndex     <=     writerIndex             size

注意，readable bytes 空间才是要服务端要发送的数据, writable bytes空间是从socket读来的数据存放的地方。
具体的说：是write还是read都是站在buffer角度看问题的，不是站在和客户端通信的socket角度。当客户socket有数据发送过来时，
socket处于可读，所以我们要把socket上的可读数据写如到buffer中缓存起来，所以对于buffer来说，当socket可读时，是需要向buffer中
write数据的，因此 writable bytes 指的是用于存放socket上可读数据的空间, readable bytes同理

*/

/**
 * @b 缓存类
 * 从socket读到缓冲区的方法是使用readv先读至buffer_，
 * buffer_空间如果不够会读入到栈上65536个字节大小的空间，然后以append的
 * 方式追加入buffer_。既考虑了避免系统调用带来开销，又不影响数据的接收(理由如下)。
 */
class Buffer
{

public:
    
    static const size_t kCheapPrepend;  //预留空间,记录相关数据,用于解决粘包问题
    static const size_t kInitialSize;   //缓冲区长度,不包括预留

    explicit Buffer(size_t = kInitialSize);

    //刻度字节长度
    size_t readableBytes() const;
    //可写字节长度
    size_t writableBytes() const;
    //已读字节长度
    size_t prependableBytes() const;

    //刻度数据的起始地址
    const char *peek() const;
    //从可读区域取出数据
    void retrieve(size_t);
    //初始化budder
    void retrieveAll();

    //转成字符串
    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t);

    //扩容或移动确保能够写下数据
    void ensureWritableBytes(size_t);

    //把[data, data+len]内存上的数据添加到writable缓冲区当中
    void append(const char *, size_t);

    //返回可读的起始地址
    char *beginWrite();
    const char *beginWrite() const;

    //从fd上读数据
    ssize_t readFd(int , int *);
    //通过fd发送数据
    ssize_t writeFd(int , int *);

private:

    //vector底层数组首元素的地址 也就是数组的起始地址
    char *begin();
    const char *begin() const;

    //调整可写空间
    void makeSpace(size_t);

    //多个buffer
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};

