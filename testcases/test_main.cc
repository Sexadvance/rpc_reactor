#include<unistd.h>
#include <signal.h>
#include "rocket/common/config.h"
#include "rocket/common/log.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"
#include "test_impl.h"

int main(int argc ,char *argv[]){
    if(argc != 2){
        printf("argc != 2,please input the profiles.\n");
        printf("Sample: ./test_main ../conf/rocket.xml\n");
        return -1;
    }

    rocket::Config::setGlobalConfig(argv[1]);

    rocket::Logger::InitGlobalLogger();

    ////////////////////////////////////////////////////////////////////////////////
    //create the service and register the service

    std::shared_ptr<Impl> service = std::make_shared<Impl>();
    rocket::RpcDispatcher::GetRpcDispatcher()->registerService(service);

    ////////////////////////////////////////////////////////////////////////////////


    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>(rocket::Config::GetGlobalConfig()->m_ip,rocket::Config::GetGlobalConfig()->m_port);
    DEBUGLOG("create addr %s",addr->toString().c_str());

    rocket::TcpServer server(addr);

    server.start();

    return 0;
}