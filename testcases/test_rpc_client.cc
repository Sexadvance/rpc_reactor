#include <assert.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <google/protobuf/service.h>
#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "order.pb.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/rpc/rpc_closure.h"

void test_tcp_client()
{
    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1" ,12345);

    rocket::TcpClient client(addr);
    client.connect([addr,&client](){
        DEBUGLOG("connect to [%s] success",addr->toString().c_str());
        std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
        message->m_msg_id = "99998888";
        message->m_pb_data = "test_pb_data";

        makeOrderRequest request;
        request.set_price(100);
        request.set_goods("apple");

        if(!request.SerializePartialToString(&(message->m_pb_data)))
        {
            ERRORLOG("serialize error");
            return;
        }

        message->m_method_name = "Order.makeOrder";

        client.writeMessage(message,[request](rocket::AbstractProtocol::s_ptr msg_ptr){
            DEBUGLOG("send message successs,request[%s]",request.ShortDebugString().c_str());
        });
        client.readMessage("99998888",[](rocket::AbstractProtocol::s_ptr msg_ptr){
            std::shared_ptr<rocket::TinyPBProtocol>messages = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
            DEBUGLOG("msg_id[%s], get response %s",messages->m_msg_id.c_str(),messages->m_pb_data.c_str());
            makeOrderResponse response;

            if(!response.ParseFromString(messages->m_pb_data))
            {
                ERRORLOG("deserialize error");
                return;
            }
            DEBUGLOG("get response success,response[%s]",response.ShortDebugString().c_str());
        });
    });  
}

void test_rpc_channel()
{
    NEWRPCCHANNEL("127.0.0.1:12345",channel);

   // std::shared_ptr<makeOrderRequest> request = std::make_shared<makeOrderRequest>();

    NEWMESSAGE(makeOrderRequest, request);

    NEWMESSAGE(makeOrderResponse, response);


    request->set_price(100);
    request->set_goods("apple");

    //std::shared_ptr<rocket::RpcController> controller = std::make_shared<rocket::RpcController>();

    NEWRPCCONTROLLER(controller);

    controller->SetMsgId("99998888");
    controller->SetTimeout(10000);

    std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>([request,response,channel,controller]()mutable{
        if(controller->GetErrorCode() ==0)
        {
            INFOLOG("call rpc success,request[%s], response[%s]",request->ShortDebugString().c_str(),response->ShortDebugString().c_str());
            //执行业务逻辑
            if(response->order_id() == "xxx")
            {

            }
        }
        else
        {
            ERRORLOG("call rpc failed, request[%s], error code[%d], error info[%s]",
                        request->ShortDebugString().c_str(),
                        controller->GetErrorCode(),
                        controller->GetErrorInfo().c_str());
        }
        INFOLOG("now exit eventloop");
        channel->getTcpClient()->stop();
        channel.reset();
    });

    channel->Init(controller,request,response,closure);

    Order_Stub stub(channel.get());


    //controller->SetTimeout(5000);
    stub.makeOrder(controller.get(),request.get(),response.get(),closure.get());

    CALLRPC("127.0.0.1:12345", makeOrder, controller, request, response, closure);

}



int main()
{
    rocket::Config::setGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    //test_tcp_client();
    test_rpc_channel();
    return 0;
}