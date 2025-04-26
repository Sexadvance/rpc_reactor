#include <sys/time.h>
#include <sstream>
#include <stdio.h>
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include "rocket/net/eventloop.h"
#include "rocket/common/run_time.h"

namespace rocket
{

static Logger* g_logger = nullptr;

Logger* Logger::GetGlobalLogger()
{
    // if(!g_logger)
    // {
    //     LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level);
    //     g_logger = new Logger(global_log_level);
    //     printf("init log level [%s]\n",LogLevelToString(global_log_level).c_str());
    // }

    return g_logger;
}

Logger::Logger(LogLevel level, int type /*= 1*/):m_set_level(level), m_type(type)
{
    if(m_type == 0) return;
    m_async_logger = std::make_shared<AsyncLogger>(
        Config::GetGlobalConfig()->m_log_file_name +"_rpc",
        Config::GetGlobalConfig()->m_log_file_path,
        Config::GetGlobalConfig()->m_log_max_file_size
        );

    m_async_app_logger = std::make_shared<AsyncLogger>(
        Config::GetGlobalConfig()->m_log_file_name +"_app",
        Config::GetGlobalConfig()->m_log_file_path,
        Config::GetGlobalConfig()->m_log_max_file_size
        );      
}

void Logger::init()
{
    if(m_type == 0) return;
    m_timer_event = std::make_shared<TimerEvent>(Config::GetGlobalConfig()->m_log_sync_interval,true, std::bind(&Logger::syncLoop,this));
    EventLoop::GetCurrentEventLoop()->addTimerEvent(m_timer_event);
}

void Logger::syncLoop()
{
    //同步 m_buffer 到 async_logger 的buffer队尾
    std::vector<std::string> tmp_vec;
    ScopeMutex<Mutex> lock(m_mutex);
    m_buffer.swap(tmp_vec);
    lock.unlock();
    if(!tmp_vec.empty())
    {
        m_async_logger->pushLogBuffer(tmp_vec);
    }
    tmp_vec.clear();

    //同步 m_app_buffer 到 app_async_logger 的buffer队尾
    std::vector<std::string> tmp_vec2;
    {
        ScopeMutex<Mutex> lock(m_mutex);
        m_app_buffer.swap(tmp_vec2);
    }
    if(!tmp_vec2.empty())
    {
        m_async_app_logger->pushLogBuffer(tmp_vec2);
    }
}



void Logger::InitGlobalLogger(int type /*=1*/)
{
    LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level);
    g_logger = new Logger(global_log_level,type);
    printf("init log level [%s]\n",LogLevelToString(global_log_level).c_str());

    g_logger->init();
}



std::string LogLevelToString(LogLevel level)
{
    switch (level)
    {
        case Debug:
            return "DEBUG";
        case Info:
            return "INFO";
        case Error:
            return "ERROR";
        default:
            return "UBKNOWN";
    }
    
}

LogLevel StringToLogLevel(const std::string& level)
{
    if(level == "DEBUG") return Debug;
    else if(level == "INFO") return Info;
    else if(level == "ERROR") return Error;
    else return Unknown;
}

std::string LogEvent::toString()
{
    struct timeval now_time;

    gettimeofday(&now_time,nullptr);

    struct tm now_time_t;
    localtime_r(&(now_time.tv_sec),&now_time_t);

    char buf[128];
    strftime(&buf[0],128,"%y-%m-%d %H:%M:%S",&now_time_t);
    std::string time_str(buf);

    int ms = now_time.tv_usec / 1000;
    time_str = time_str + "." + std::to_string(ms);

    m_pid = getPid();
    m_thread_id = getThreadId();

    std::stringstream ss;

    ss << "[" << LogLevelToString(m_level) << "]\t"
       << "[" <<time_str << "]\t"
       << "[" << m_pid << ":" << m_thread_id << "]\t";

    //获取当前线程处理请求的msgid
    std::string msg_id = RunTime::GetRunTime()->m_msg_id;
    std::string method_name = RunTime::GetRunTime()->m_method_name;

    if(!msg_id.empty())
    {
        ss << "[" << msg_id << "]\t";
    }
    if(!method_name.empty())
    {
        ss << "[" << method_name << "]\t";
    }

    return ss.str();

}

void Logger::pushLog(const std::string& msg)
{
    if(m_type == 0)
    {
        printf("%s\n",msg.c_str());
        return;
    }
    ScopeMutex<Mutex>lock(m_mutex);
    m_buffer.push_back(msg);
}

void Logger::pushAppLog(const std::string& msg)
{
    ScopeMutex<Mutex>lock(m_app_mutex);
    m_app_buffer.push_back(msg);
}

void Logger::log()
{
    // std::queue<std::string> tmp;
    // {
    //     ScopeMutex<Mutex>lock(m_mutex);
    //     m_buffer.swap(tmp);
    // }
    // while(!tmp.empty())
    // {
    //     std::string msg = tmp.front();
    //     tmp.pop();

    //     printf("%s",msg.c_str());
    // }




}

AsyncLogger::AsyncLogger(const std::string& file_name, std::string& file_path, int max_file_size)
:m_file_name(file_name),m_file_path(file_path),m_max_file_size(max_file_size)
{
    sem_init(&m_sempahore,0,0);

    pthread_cond_init(&m_conditon, NULL);

    pthread_create(&m_thread, NULL, &AsyncLogger::loop, this);

    sem_wait(&m_sempahore);
}

void* AsyncLogger::loop(void* arg)
{
    //将buffer里面的全部数据打印到文件中，然后线程睡眠，直到有新的数据再重复这个过程

    AsyncLogger* logger = reinterpret_cast<AsyncLogger*>(arg);

    sem_post(&logger->m_sempahore);

    while(1)
    {
        ScopeMutex<Mutex> lock(logger->m_mutex);
        while(logger->m_buffer.empty())
        {
            pthread_cond_wait(&logger->m_conditon, logger->m_mutex.getMutex());
        }

        std::vector<std::string> tmp;
        tmp.swap(logger->m_buffer.front());
        logger->m_buffer.pop();

        lock.unlock();

        timeval now;
        gettimeofday(&now, NULL);

        struct tm now_time;
        localtime_r(&(now.tv_sec), &now_time);

        const char* format = "%Y%m%d";

        char date[32];
        strftime(date, sizeof(date), format, &now_time);

        if(std::string(date) != logger->m_date)
        {
            logger->m_no = 0;
            logger->m_reopen_flag = true;
            logger->m_date = std::string(date);
        }

        if(logger->m_file_handler == NULL)
        {
            logger->m_reopen_flag = true;
        }

        std::stringstream ss;
        ss << logger->m_file_path << logger->m_file_name << "_" << std::string(date) << "_";

        std::string log_file_name = ss.str() + std::string("log") + std::string(".") + std::to_string(logger->m_no) ;

        if(logger->m_reopen_flag)
        {
            if(logger->m_file_handler)
            {
                fclose(logger->m_file_handler);
            }
            logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            logger->m_reopen_flag = false;
        }
        
        //ftell，c标准函数，获取文件偏移量
        if(ftell(logger->m_file_handler) > logger->m_max_file_size)
        {
            fclose(logger->m_file_handler);

            log_file_name = ss.str() + std::to_string(++logger->m_no) + std::string(".log");
            logger->m_file_handler = fopen(log_file_name.c_str(), "a");
            logger->m_reopen_flag = false;
        }

        for(auto& i : tmp)
        {
            if(!i.empty())
            {
                fwrite(i.c_str(), 1, i.length(), logger->m_file_handler);
            }
        }
        fflush(logger->m_file_handler);

        if(logger->m_stop_flag)
        {
            break;
        }
    }
    return NULL;
}

void AsyncLogger::stop()
{
    m_stop_flag = true;
}

void AsyncLogger::flush()
{
    if(m_file_handler)
    {
        fflush(m_file_handler);
    }
}

void AsyncLogger::pushLogBuffer(std::vector<std::string>& vec)
{
    ScopeMutex<Mutex> lock(m_mutex);
    m_buffer.push(vec);
    lock.unlock();

    //唤醒异步日志线程
    pthread_cond_signal(&m_conditon);
}




}







