#include <sys/time.h>
#include <sstream>
#include <stdio.h>
#include "rocket/common/log.h"
#include "rocket/common/util.h"


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

void Logger::InitGlobalLogger()
{
    LogLevel global_log_level = StringToLogLevel(Config::GetGlobalConfig()->m_log_level);
    g_logger = new Logger(global_log_level);
    printf("init log level [%s]\n",LogLevelToString(global_log_level).c_str());
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

    return ss.str();

}

void Logger::pushLog(const std::string& msg)
{
    ScopeMutex<Mutex>lock(m_mutex);
    m_buffer.push(msg);
}

void Logger::log()
{
    std::queue<std::string> tmp;
    {
        ScopeMutex<Mutex>lock(m_mutex);
        m_buffer.swap(tmp);
    }
    while(!tmp.empty())
    {
        std::string msg = tmp.front();
        tmp.pop();

        printf("%s",msg.c_str());
    }
}


}







