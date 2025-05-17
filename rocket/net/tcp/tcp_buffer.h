#ifndef ROCKET_NET_TCP_TCP_BUFFER_H
#define ROCKET_NET_TCP_TCP_BUFFER_H
#include <vector>
#include <memory>
namespace rocket
{
 
class TcpBuffer
{
public:
    typedef std::shared_ptr<TcpBuffer> s_ptr;
    TcpBuffer(int size);

    ~TcpBuffer();

    //返回可读字节数
    int readAble();


    //返回可写字节数
    int writeAble();

    //返回可读的首地址
    int readIndex();

    //返回可写的首地址
    int writeIndex();

    void writeToBuffer(const char* buf,int size);

    void readFromBuffer(std::vector<char>& re,int size);

    void resizeBuffer(int new_size);

    void adjustBuffer();

    //删除可读的内容
    void moveReadIndex(int size);

    //删除可写的内容
    void moveWriteIndex(int size);

private:
    int m_read_index {0};//可读内容的首地址
    int m_write_index {0};//可写内容的首地址
    int m_size{0};

public:
    std::vector<char> m_buffer; 
};



}
#endif