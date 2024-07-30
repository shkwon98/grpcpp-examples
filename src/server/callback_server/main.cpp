// standard headers
#include <iostream>
#include <memory>
#include <string>

// grpc headers
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <robl/api/service.grpc.pb.h>

using robl::api::HelloRequest;
using robl::api::HelloResponse;
using robl::api::TestService;

// Logic and data behind the server's behavior.
class TestServiceImpl final : public TestService::CallbackService
{
    grpc::ServerUnaryReactor *SayHello(grpc::CallbackServerContext *context, const HelloRequest *request,
                                       HelloResponse *reply) override
    {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());

        grpc::ServerUnaryReactor *reactor = context->DefaultReactor();
        reactor->Finish(grpc::Status::OK);
        return reactor;
    }
};

void RunServer(void)
{
    const auto &server_address = std::string("localhost:50051");
    TestServiceImpl service;

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(void)
{
    RunServer();
    return 0;
}
