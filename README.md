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
### 2.4.1 TimerEvent 定时任务
``` 
1.指定时间点 arrive_time
2.interval,ms间隔
3.is_repeated 是否是周期性的
4.is_cancled
5.task

cancle()
canclerepeated()
```

### Timer
定时器，它是一个TimerEvent集合
Timer 集成 FdEvent
```
addTimerEvent();
deleteTimerEvent();
onTimer();   //当发生了I/O时间之后，需要执行的方法

reserArriveTime()

multimap 存储 TimerEvent <key(arrivetime),TimerEvent>
```

### 2.5 IO 线程
创建一个IO线程，他会帮我们执行
1.创建一个新线程(pthread_create)
2.在新线程里面，创建一个 Eventloop ,完成完成初始化
3.开启 loop
```
class
{
    
}

```

### RPC服务端流程
```
启动的时候注册RPC对象

1.从buffer读取数据，decode得到请求的TinyPBProtcol对象。然后从请求的TinyPBProtcol对象得到method_name，从orderService对象里根据service.method_name找到方法func
2.找到对应的request type和response type
3.将请求体TinyPBProtocol里面的pb_data反序列化为request type对象，声明一个response type对象
4.func(request,response)
5.将response对象序列化为pb_data,再塞入到TinyPBProtocol结构体中，进行encode然后塞入buffer里面，发送回包
```

3.1.1. protobuf

**protobuf** 是 **google** 开源的有名的序列化库。谷歌出品，必属精品！**TinyRPC** 的 **TinyPB** 协议是基于 protobuf 来 序列化/反序列化 的，因此这个库是必须的。

其地址为：https://github.com/protocolbuffers/protobuf

推荐安装版本 **3.19.4** 及以上。安装过程不再赘述, **注意将头文件和库文件 copy 到对应的系统路径下。**

### 3.1.2. tinyxml

由于 **TinyRPC** 读取配置使用了 **xml** 文件，因此需要安装 **tinyxml** 库来解析配置文件。

下载地址：https://sourceforge.net/projects/tinyxml/

要生成 libtinyxml.a 静态库，需要简单修改 makefile 如下:

```
# 84 行修改为如下

OUTPUT := libtinyxml.a



# 194, 105 行修改如下

${OUTPUT}: ${OBJS}

  ${AR} $@ ${LDFLAGS} ${OBJS} ${LIBS} ${EXTRA_LIBS}
```

安装过程如下：

```
cd tinyxml

make -j4



# copy 库文件到系统库文件搜索路径下

cp libtinyxml.a /usr/lib/



# copy 头文件到系统头文件搜索路径下

mkdir /usr/include/tinyxml

cp *.h /usr/include/tinyxml
```