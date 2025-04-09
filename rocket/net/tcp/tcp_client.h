#ifndef ROCKET_NET_TCP_TCP_CLIENT_H
#define ROCKET_NET_TCP_TCP_CLIENT_H

#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/tcp/abstract_protocol.h"

namespace rocket
{
class TcpClient
{
public:
    TcpClient(NetAddr::s_ptr peer_addr);

    ~TcpClient();

    //异步的进行connect
    // 如果connect成功，done会被执行
    void connect(std::function<void()> done);

    //异步的发送message
    //如果发送message成功，会调用don函数，函数的入参就是message对象
    void writeMessage(AbstractProtocol::s_ptr request,std::function<void(AbstractProtocol::s_ptr)> done);

    //异步的发读取message
    //如果读取message成功，会调用don函数，函数的入参就是message对象
    void readMessage(AbstractProtocol::s_ptr request,std::function<void(AbstractProtocol::s_ptr)> done);



private:
    NetAddr::s_ptr m_peer_addr;
    EventLoop* m_event_loop{NULL};

    int m_fd{-1};
    FdEvent* m_fd_event{NULL};
    TcpConnection::s_ptr m_conncection;
};
}

#endif