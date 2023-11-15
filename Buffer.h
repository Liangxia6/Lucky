#pragma once

#include <vector>
#include <string>

class Buffer{

public:
    
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;

    explicit Buffer(size_t = kInitialSize);

    size_t readableBytes() const;
    size_t writableBytes() const;
    size_t prependableBytes() const;

    const char *peek() const;
    void retrieve(size_t);
    void retrieveAll();

    std::string retrieveAllAsString();
    std::string retrieveAsString(size_t);

    void ensureWritableBytes(size_t);

    void append(const char *, size_t);

    char *beginWrite();
    const char *beginWrite() const;

    ssize_t readFd(int , int *);

    ssize_t writeFd(int , int *);

private:

    char *begin();
    const char *begin() const;

    void makeSpace(size_t);

    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;

};
