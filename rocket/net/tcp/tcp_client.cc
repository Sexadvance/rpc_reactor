#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include "rocket/common/log.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/common/error_code.h"
#include "rocket/net/tcp/net_addr.h"

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
        initLocalAddr();
        m_conncection->setState(Connected);
        if(done)
        {
            done();
        }
    }
    else if(rt == -1)
    {
        if(errno == EINPROGRESS)
        {
            m_fd_event->listen(FdEvent::OUT_EVENT,
                [this,done]()
                {

                    // epoll监听可写事件，然后判断错误码
                    // int error = 0;
                    // socklen_t error_len = sizeof(error);
                    // getsockopt(m_fd,SOL_SOCKET,SO_ERROR,&error,&error_len);
                    // if(error == 0)
                    // {
                    //     DEBUGLOG("connect [%s] success",m_peer_addr->toString().c_str());
                    //     initLocalAddr();
                    //     m_conncection->setState(Connected);
                    // }
                    // else
                    // {
                    //     m_connect_error_code = ERROR_FAILED_CONNECT;
                    //     m_connect_error_info = "connect errors, sys error = " + std::string(strerror(errno));
                    //     ERRORLOG("connect error,errno=%d,error%s",errno,strerror(errno));
                    // }


                    int rt = ::connect(m_fd,m_peer_addr->getSockAddr(),m_peer_addr->getSockLen());

                    if((rt < 0 && errno == EISCONN) || (rt == 0))
                    {
                        DEBUGLOG("connect [%s] success",m_peer_addr->toString().c_str());
                        initLocalAddr();
                        m_conncection->setState(Connected);
                    }
                    else
                    {
                        if(errno == ECONNREFUSED)
                        {
                            m_connect_error_code = ERROR_PEER_CLOSE;
                            m_connect_error_info = "connect refused, sys error = " + std::string(strerror(errno));
                        }
                        else
                        {
                            m_connect_error_code = ERROR_FAILED_CONNECT;
                            m_connect_error_info = "connect unknown error, sys error = " + std::string(strerror(errno));
                        }
                        DEBUGLOG("connect error, errno=%d, error=%s",errno,strerror(errno));
                        close(m_fd);
                        m_fd = socket(m_peer_addr->getFamily(),SOCK_STREAM,0);
                    }


                    //去掉事件的监听 
                    m_event_loop->deleteEpollEvent(m_fd_event);
                    DEBUGLOG("now begin to done");

                    //如果连接成功，才会执行回调函数
                    if(done)
                    {
                        done();
                    }
                },
                [this,done]()
                {
                    if(errno == ECONNREFUSED)
                    {
                        m_connect_error_code = ERROR_FAILED_CONNECT;
                        m_connect_error_info = "connect errors, sys error = " + std::string(strerror(errno));
                        ERRORLOG("connect error,errno=%d,error%s",errno,strerror(errno));
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

            m_connect_error_code = ERROR_FAILED_CONNECT;
            m_connect_error_info = "connect errors, sys error = " + std::string(strerror(errno));

            if(done)
            {
                done();
            }
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
void TcpClient::readMessage(const std::string& msg_id,std::function<void(AbstractProtocol::s_ptr)> done)
{
    //1.监听可读事件
    //2.从buffer事件里 decode 得到 message 对象。判断是否msg_id相等，相等则读成功，执行其回调
    m_conncection->pushReadMessage(msg_id,done);
    m_conncection->listenRead();

}

void TcpClient::stop()
{
    if(m_event_loop->isLooping())
    {
        m_event_loop->stop();
    }
}

void TcpClient::shutdown()
{
    m_conncection->shutdown();
}


int  TcpClient::getConnectErrorCode()
{
    return m_connect_error_code;

}

std::string  TcpClient::getConnectErrorInfo()
{
    return m_connect_error_info;

}

NetAddr::s_ptr TcpClient::getPeerAddr()
{
    return m_peer_addr;
}
NetAddr::s_ptr TcpClient::getLocalAddr()
{
    return m_local_addr;
}

void TcpClient::initLocalAddr()
{
    sockaddr_in local_addr;
    socklen_t len = sizeof(local_addr);
    int ret = getsockname(m_fd,reinterpret_cast<sockaddr*>(&local_addr),&len);
    if(ret != 0)
    {
        ERRORLOG("initLocalAddr error,getsockname error.errno=%d,error=%s",errno,strerror(errno));
        return;
    }
    m_local_addr = std::make_shared<IPNetAddr>(local_addr);
}

void TcpClient::addTimerEvent(TimerEvent::s_ptr timer_event)
{
    m_event_loop->addTimerEvent(timer_event);
}

}