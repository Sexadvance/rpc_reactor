# tinyrpc
a tinyframework 

### 日志模块
1.日志级别
2.打印到文件，支持日期命令，以及日志的滚动
3.C格式化风格
4.线程安全

loglevel
```
Debug 
Info
Error
```
LogEvent:
```
文件名、行号 
MsgNo
进程号
Thread id
日期、时间。精确到毫秒。
自定义消息 
```

日志格式
```
[Level][%y-%m-%d %H:%m:%S.%ms]\t[pid::thread_id]\t[filename::line][%msg]
```

Logger 日志器
1.提供打印日志的方法
2.设置日志输出的路径

```creator模型

void loop
{
    while(!stop)
    {
        //1.取得下次定时任务的时间
        int time_out = Max(1000,getNextTimerCallBack());
        //2.调用epoll_wait()等待事件发生
        int rt = epollwait()
        if(rt  < 0)
        {
            
        }
    }


}

```

