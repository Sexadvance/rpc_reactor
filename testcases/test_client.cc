#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

void test_connect()
{
    //调用 connect  连接 server
    //write 一个字符串
    //等待 read 返回结果
    int fd = socket(AF_INET,SOCK_STREAM,0);

    if(fd < 0)
    {
        ERRORLOG("invalid fd %d",fd);
        exit(0);
    }

    sockaddr_in server_addr;
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_aton("127.0.0.1",&server_addr.sin_addr);

    int rt = connect(fd,reinterpret_cast<sockaddr*>(&server_addr),sizeof(server_addr));

    std::string msg = "hello rocket!";

    rt = write(fd,msg.c_str(),msg.length());

    DEBUGLOG("success write %d bytes,[%s]",rt,msg.c_str());

    char buf[100];
    memset(buf,0,sizeof(buf));
    rt = read(fd,buf,100);

    DEBUGLOG("success read %d bytes,[%s]",rt,std::string(buf).c_str());
}

void test_tcp_client()
{
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1" ,12345);

    rocket::TcpClient client(addr);
    client.connect([addr,&client](){
        DEBUGLOG("connect to [%s] success",addr->toString().c_str());
        std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
        message->m_msg_id = "123456789";
        message->m_pb_data = "test_pb_data";
        client.writeMessage(message,[](rocket::AbstractProtocol::s_ptr msg_ptr){
            DEBUGLOG("send message successs");
        });
        client.readMessage("123456789",[](rocket::AbstractProtocol::s_ptr msg_ptr){
            std::shared_ptr<rocket::TinyPBProtocol>messages = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("req_id[%s], get response %s",messages->m_msg_id.c_str(),messages->m_pb_data.c_str());
        });
    });  
}

int main()
{
    rocket::Config::setGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    test_connect();

    //test_tcp_client();
    return 0;
}