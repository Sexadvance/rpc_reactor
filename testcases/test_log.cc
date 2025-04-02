#include "rocket/common/log.h"
#include "rocket/common/config.h"

void* func(void *)
{
    DEBUGLOG("this is thread in %s","func");
    INFOLOG("test info log %s","11");
    return nullptr;
}

int main()
{
    rocket::Config::setGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();
    pthread_t thread;
    pthread_create(&thread,NULL,func,NULL);
    DEBUGLOG("test debuglog %s","11"); 
    INFOLOG("test info log %s","11");
    pthread_join(thread,NULL);
    return 0;
}