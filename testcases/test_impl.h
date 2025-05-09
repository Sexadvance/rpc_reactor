#include "${PROJECT_NAME}.pb.h"
#include <google/protobuf/service.h>


class Impl : public Order{
public:
    void makeOrder(google::protobuf::RpcController* controller,
        const ::makeOrderRequest* request,
        ::makeOrderResponse* response,
        ::google::protobuf::Closure* done);
    void ${SERVICE_NAME}
};