#include "rocket/common/log.h"
#include "rocket/net/tcp/net_addr.h"



int main()
{
    rocket::Config::setGlobalConfig("../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();

    rocket::IPNETAddr addr("127.0.0.1",12345);
    DEBUGLOG("craete addr %s",addr.toString().c_str());
}