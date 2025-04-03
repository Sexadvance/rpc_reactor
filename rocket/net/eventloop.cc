#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>
#include "rocket/net/eventloop.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

#define ADD_TO_EPOLL()\
auto it = m_listen_fds.find(event->getFd());\
int op = EPOLL_CTL_ADD;\
if(it != m_listen_fds.end())\
{\
    op = EPOLL_CTL_MOD;\
}\
epoll_event tmp = event->getEpollEvent();\
int rt = epoll_ctl(m_epoll_fd,op,event->getFd(),&tmp);\
if(rt != 0)\
{\
    ERRORLOG("fail to epoll ctl when add fd = %d,errno = %d,error = %d",event->getFd(),errno,strerror(errno));\
}\
DEBUGLOG("add event success,fd[%d]",event->getFd())\

#define DELETE_TO_EPOLL()\
auto it = m_listen_fds.find(event->getFd());\
if(it == m_listen_fds.end())\
{\
    return;\
}\
int op = EPOLL_CTL_DEL;\
epoll_event tmp = event->getEpollEvent();\
int rt = epoll_ctl(m_epoll_fd,op,event->getFd(),&tmp);\
if(rt == -1)\
{\
    ERRORLOG("fail to epoll ctl when add fd = %d,errno = %d,error = %s",event->getFd(),errno,strerror(errno));\
}\
DEBUGLOG("del event success,fd[%d]",event->getFd()) \



namespace rocket
{

static thread_local EventLoop*  t_current_eventloop = NULL;

static int g_epoll_max_timeout = 10000;
static int g_epoll_max_events = 10;

EventLoop::EventLoop()
{
    if(t_current_eventloop != NULL)
    {
        ERRORLOG("failed to create event loop, this thread has createdd event loop");
        exit(0);
    }

    m_thread_id = getThreadId();

    m_epoll_fd = epoll_create(1);

    if(m_epoll_fd == -1)
    {
        ERRORLOG("failed to create event loop,epoll_create error ,error info[%d]",errno);
        exit(0);
    }

    initWakeUpFdEvent();
    INFOLOG("succ create event loop in thread %d",m_thread_id);
    t_current_eventloop =this;
}
EventLoop::~EventLoop()
{
    close(m_epoll_fd);
    if(m_wakeup_fd_event!= NULL)
    {
        delete m_wakeup_fd_event;
        m_wakeup_fd_event = NULL;
    }
}

void EventLoop::initWakeUpFdEvent()
{
    m_wakep_fd = eventfd(0,EFD_NONBLOCK);
    if(m_wakep_fd < 0)
    {
        ERRORLOG("failed to create loop, eventfd create error,error info[%d]",errno);
        exit(0);
    }

    m_wakeup_fd_event = new WakeUpFdEvent(m_wakep_fd);

    m_wakeup_fd_event->listen(FdEvent::IN_EVENT,[this](){
        char buf[8];
        while(read(m_wakep_fd,buf,8) != -1 && errno != EAGAIN){}
        DEBUGLOG("read full bytes from wakeup fd[%d]",m_wakep_fd);
    });
    
    addEpollEvent(m_wakeup_fd_event);
}

void EventLoop::loop()
{
    while(!m_stop_flag)
    {
        std::queue<std::function<void()>> tmp_tasks;
        {
            ScopeMutext<Mutex> lock(m_mutex);
            tmp_tasks.swap(m_pending_tasks);
        }

        while(!tmp_tasks.empty())
        {
            std::function<void()> cb = tmp_tasks.front();
            tmp_tasks.pop();
            if(cb!=NULL)
            {
                cb();
            }
        }

        int timeout = g_epoll_max_timeout;
        epoll_event result_events[g_epoll_max_events];
        //DEBUGLOG("now begin to epoll wait");
        int rt = epoll_wait(m_epoll_fd,result_events,g_epoll_max_events,timeout);
        DEBUGLOG("now end to epoll wait,rt = %d",rt);

        if(rt < 0)
        {
            ERRORLOG("epoll_wait error,errno=%d",errno);
        }
        else
        {
            for(int i = 0;i < rt; i++)
            {
                epoll_event trigger_event = result_events[i];
                FdEvent* fd_event = static_cast<FdEvent*> (trigger_event.data.ptr);
                if(fd_event == NULL)
                {
                    continue;
                }
                if(trigger_event.events & EPOLLIN)
                {
                    DEBUGLOG("fd %d trigger EPOLLIN event",fd_event->getFd());
                    addTask(fd_event->handler(FdEvent::IN_EVENT));
                }
                if(trigger_event.events & EPOLLOUT)
                {
                    DEBUGLOG("fd %d trigger EPOLLOUT event",fd_event->getFd());
                    addTask(fd_event->handler(FdEvent::OUT_EVENT));
                }
            }
        }

    }
}

void EventLoop::wakeup()
{
    m_wakeup_fd_event->wakeup();
}

void EventLoop::stop()
{
    m_stop_flag = true;
}

void EventLoop::dealwakeup()
{

}

void EventLoop::addEpollEvent(FdEvent* event)
{
    if(isInLoopThread())
    {
        ADD_TO_EPOLL();
    }
    else
    {
        auto cb = [this,event]()
        {
            ADD_TO_EPOLL();
        };
        addTask(cb,true);
    }
}


void EventLoop::deleteEpollEvent(FdEvent* event)
{
    if(isInLoopThread())
    {
        DELETE_TO_EPOLL();
    }
    else
    {
        auto cb = [this,event]()
        {
            DELETE_TO_EPOLL();
        };
        addTask(cb,true);
    }
}

void EventLoop::addTask(std::function<void()> cb,bool is_wake_up /*=false*/)
{
    {
        ScopeMutext<Mutex> lock(m_mutex);
        m_pending_tasks.push(cb);
    }
    if(is_wake_up)
    {
        wakeup();
    }
}

bool EventLoop::isInLoopThread()
{
    return getThreadId() == m_thread_id;
}

}