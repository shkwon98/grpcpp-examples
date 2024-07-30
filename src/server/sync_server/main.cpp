// standard headers
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// grpc headers
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <robl/api/service.grpc.pb.h>

using robl::api::ChatRequest;
using robl::api::ChatResponse;
using robl::api::HelloRequest;
using robl::api::HelloResponse;
using robl::api::SubscribeProgressRequest;
using robl::api::SubscribeProgressResponse;
using robl::api::TestService;

// Logic and data behind the server's behavior.
class TestServiceImpl final : public TestService::Service
{
    grpc::Status SayHello(grpc::ServerContext *context, const HelloRequest *request, HelloResponse *reply) override
    {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        return grpc::Status::OK;
    }

    grpc::Status SubscribeProgress(grpc::ServerContext *context, const SubscribeProgressRequest *request,
                                   grpc::ServerWriter<SubscribeProgressResponse> *writer) override
    {
        SubscribeProgressResponse response;
        const int total_steps = 19;
        for (int i = 0; i <= total_steps; ++i)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));

            response.set_progress(100.0 * i / total_steps);
            writer->Write(response);
        }
        return grpc::Status::OK;
    }

    grpc::Status Chat(grpc::ServerContext *context, grpc::ServerReaderWriter<ChatResponse, ChatRequest> *stream) override
    {
        ChatRequest request;
        ChatResponse response;
        while (stream->Read(&request))
        {
            response.set_message("You said: " + request.message());
            stream->Write(response);
        }
        return grpc::Status::OK;
    }
};

void RunServer(void)
{
    const auto &server_address = std::string("localhost:50051");
    auto service = std::make_shared<TestServiceImpl>();

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());

    auto server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(void)
{
    RunServer();

    return 0;
}
