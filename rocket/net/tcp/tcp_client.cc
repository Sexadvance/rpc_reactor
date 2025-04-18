#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/fd_event_group.h"

namespace rocket
{
TcpClient::TcpClient(NetAddr::s_ptr peer_addr):m_peer_addr(peer_addr)
{
    m_event_loop = EventLoop::GetCurrentEventLoop();
    m_fd = socket(peer_addr->getFamily(),SOCK_STREAM,0);

    if(m_fd < 0)
    {
        ERRORLOG("TcpClient::TcpClient() error,failed to create fd");
        return;
    }

    m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(m_fd);
    m_fd_event->setNonBlock();

    m_conncection = std::make_shared<TcpConnection>(m_event_loop,m_fd,128,nullptr,peer_addr,TcpConnectionByClient);
    m_conncection->setConnectionTyepe(TcpConnectionByClient);
}

TcpClient::~TcpClient()
{
    if(m_fd >0)
    {
        close(m_fd);
    }
}
//异步的进行connect
// 如果connect成功，done会被执行
void TcpClient::connect(std::function<void()> done)
{
    int rt =::connect(m_fd,m_peer_addr->getSockAddr(),m_peer_addr->getSockLen());
    if(rt == 0)
    {
        DEBUGLOG("connect success");
        if(done)
        {
            done();
        }
    }
    else if(rt == -1)
    {
        if(errno == EINPROGRESS)
        {
            m_fd_event->listen(FdEvent::OUT_EVENT,[this,done](){
                // epoll监听可写事件，然后判断错误码
                int error = 0;
                socklen_t error_len = sizeof(error);
                getsockopt(m_fd,SOL_SOCKET,SO_ERROR,&error,&error_len);
                bool is_connect_succ = false;
                if(error == 0)
                {
                    DEBUGLOG("connect [%s] success",m_peer_addr->toString().c_str());
                    is_connect_succ = true;
                    m_conncection->setState(Connected);
                }
                else
                {
                    ERRORLOG("connect error,errno=%d,error%s",errno,strerror(errno));
                }
                //需要去掉可写事件的监听，不然会一直触发 
                m_fd_event->cancel(FdEvent::IN_EVENT);
                m_event_loop->addEpollEvent(m_fd_event);

                //如果连接成功，才会执行回调函数
                if(is_connect_succ && done)
                {
                    done();
                }

            });
            m_event_loop->addEpollEvent(m_fd_event);
            if(!m_event_loop->isLooping())
            {
                m_event_loop->loop();
            }
        }
        else
        {
            ERRORLOG("connect errnor,errno=%d,error=%s",errno,strerror(errno));
        }
    }
}

//异步的发送message
//如果发送message成功，会调用done函数，函数的入参就是message对象
void TcpClient::writeMessage(AbstractProtocol::s_ptr message,std::function<void(AbstractProtocol::s_ptr)> done)
{
    //1.把message对象写入到 connection 的 buffer, done 也写入
    //2.启动 connection 可写事件
    m_conncection->pushSendMessage(message,done);
    m_conncection->listenWrite();
     
}

//异步的发读取message
//如果读取message成功，会调用done函数，函数的入参就是message对象
void TcpClient::readMessage(const std::string& req_id,std::function<void(AbstractProtocol::s_ptr)> done){
{
    //1.监听可读事件
    //2.从buffer事件里 decode 得到 message 对象。判断是否req_id相等，相等则读成功，执行其回调
    m_conncection->pushReadMessage(req_id,done);
    m_conncection->listenRead();

}

}
}