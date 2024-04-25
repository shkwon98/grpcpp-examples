// standard headers
#include <iostream>
#include <memory>
#include <string>
#include <thread>

// grpc headers
#include <grpcpp/grpcpp.h>

// project headers
#include "test_service/test_service_impl.hpp"

using grpc::Server;
using grpc::ServerBuilder;

void RunServer()
{
    const std::string server_address("0.0.0.0:50051");
    auto test_service = std::make_shared<TestServiceImpl>();

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(test_service.get());

    auto server = builder.BuildAndStart();
    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}

int main()
{
    RunServer();
    return 0;
}
