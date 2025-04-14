#include "Buffer.h"

const size_t Buffer::kCheapPrepend = 8;
const size_t Buffer::kInitialSize = 1024;

Buffer::Buffer(size_t initalSize)
    //默认为8+1024
    : buffer_(kCheapPrepend + initalSize)
    , readerIndex_(kCheapPrepend)
    , writerIndex_(kCheapPrepend)
{
}

size_t Buffer::readableBytes() const 
{ 
    return writerIndex_ - readerIndex_; 
}

size_t Buffer::writableBytes() const 
{ 
    return buffer_.size() - writerIndex_; 
}

size_t Buffer::prependableBytes() const 
{ 
    return readerIndex_; 
}

//返回缓冲区中可读数据的起始地址
const char *Buffer::peek() const 
{ 
    return begin() + readerIndex_; 
}

//从可读区域中取出数据
void Buffer::retrieve(size_t len){
    if (len < readableBytes())
    {
        readerIndex_ += len; 
    }
    else
    {
        retrieveAll();
    }
}

//就是把readerIndex_和writerIndex_移动到初始位置
void Buffer::retrieveAll()
{
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
}

//把onMessage函数上报的Buffer数据, 转成string类型的数据返回
std::string Buffer::retrieveAllAsString()
{
    return retrieveAsString(readableBytes()); 
}

std::string Buffer::retrieveAsString(size_t len)
{
    std::string result(peek(), len);
    //上面一句把缓冲区中可读的数据已经读取出来 这里肯定要对缓冲区进行复位操作
    retrieve(len);
    return result;
}

void Buffer::ensureWritableBytes(size_t len)
{
    if (writableBytes() < len)
    {
        //通过移动可读数据来腾出可写空间或者直接对buffer扩容
        makeSpace(len);
    }
}

//把[data, data+len]内存上的数据添加到writable缓冲区当中
void Buffer::append(const char *data, size_t len)
{
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    writerIndex_ += len;
}

char *Buffer::beginWrite() 
{ 
    return begin() + writerIndex_; 
}

const char *Buffer::beginWrite() const 
{ 
    return begin() + writerIndex_; 
}


ssize_t Buffer::readFd(int fd, int *saveErrno)
{
    //栈额外空间，用于从套接字往出读时，当buffer_暂时不够用时暂存数据，待buffer_重新分配足够空间后，在把数据交换给buffer_
    char extrabuff[65536] = {0};                //64kb
    struct iovec vec[2];                        //使用两个缓冲区
    const size_t writable = writableBytes();    //可写缓冲区大小

    //第一块缓冲区, 指向可写空间
    //当我们用readv从socket缓冲区读数据，首先会先填满这个vec[0], 也就是我们的Buffer缓冲区
    vec[0].iov_base = begin() + writerIndex_;   
    vec[0].iov_len = writable;

    //第二块缓冲区,指向栈空间
    //如果Buffer缓冲区都填满了，那就填到我们临时创建的栈空间
    vec[1].iov_base = extrabuff;
    vec[1].iov_len = sizeof(extrabuff);

    //如果Buffer缓冲区大小比extrabuf(64k)还小，那就Buffer和extrabuf都用上
    //如果Buffer缓冲区大小比64k还大或等于，那么就只用Buffer
    const int iovcnt = (writable < sizeof(extrabuff)) ? 2 : 1;
    //Buffer存不下，剩下的存入暂时存入到extrabuf中
    const ssize_t n = readv(fd, vec, iovcnt);

    if(n < 0)
    {
        *saveErrno = errno;
    }
    //Buffer的可写缓冲区已经够存储读出来的数据了
    else if (n <= writable)
    {
        writerIndex_ += n;
    }
    //Buffer存不下，对Buffer扩容，然后把extrabuf中暂存的数据拷贝（追加）到Buffer
    else
    {
        writerIndex_ = buffer_.size();
        //根据情况对buffer_扩容 并将extrabuf存储的另一部分数据追加至buffer_
        append(extrabuff, n - writable);
    }
    return n;
}

// 注意：
// inputBuffer_.readFd表示将对端数据读到inputBuffer_中，移动writerIndex_指针
// outputBuffer_.writeFd表示将数据写入到outputBuffer_中，从readerIndex_开始，可以写readableBytes()个字节
// 具体理由见Buffer.h顶部的注释
ssize_t Buffer::writeFd(int fd, int *saveErrno)
{
    //向socket fd上写数据，假如TCP发送缓冲区满
    ssize_t n = ::write(fd, peek(), readableBytes());
    if (n < 0)
    {
        *saveErrno = errno;
    }
    return n;
}

//vector底层数组首元素的地址 也就是数组的起始地址
char *Buffer::begin() 
{ 
    //先调用begin()返回buffer_的首个元素的迭代器，然后再解引用得到这个变量的，再取地址，得到这个变量的首地址。
    return &*buffer_.begin(); 
}

const char *Buffer::begin() const 
{ 
    return &*buffer_.begin(); 
}

void Buffer::makeSpace(size_t len)
{
    /**
     * | kCheapPrepend |xxx| reader | writer |                     // xxx标示reader中已读的部分
     * | kCheapPrepend | reader ｜          len          |
     **/
    // 当len > xxx + writer的部分，即：能用来写的缓冲区大小 < 我要写入的大小len，那么就要扩容了
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
        buffer_.resize(writerIndex_ + len);
    }
    /*
    p-kC(prependable减去kCheapPrepend)的长度与writable bytes的和就是可写的长度，如果len<=可写的部分,说明还能len长的数据还能写的下，但是可写的空间是p-kC的长度与writable bytes
            共同构成，而且p-kC的长度与writable bytes不连续，所以需要将readable bytes向前移动p-kC个单位，使得可写部分空间是连续的
            调整前：
            +-------------------------+----------------------+---------------------+
            |    prependable bytes    |    readable bytes    |    writable bytes   |
            | kCheapPrepend |  p-kC   |      (CONTENT)       |                     |
            +-------------------------+----------------------+---------------------+
            |               |         |                      |                     |
            0        <=     8     readerIndex     <=     writerIndex             size
            调整后：
            +---------------+--------------------+---------------------------------+
            | prependable   |  readable bytes    |     新的writable bytes          |
            | kCheapPrepend |    (CONTENT)       | = p-kC+之前的writable bytes     |
            +---------------+--------------------+---------------------------------+
            |               |                    |                                 |
            0    <=    readerIndex    <=    writerIndex                          size
    */
    else 
    {
        size_t readable = readableBytes();
        std::copy(begin() + readerIndex_,
                begin() + writerIndex_, 
                begin() + kCheapPrepend);
        readerIndex_ = kCheapPrepend;
        writerIndex_ = readerIndex_ + readable;
    }
}

