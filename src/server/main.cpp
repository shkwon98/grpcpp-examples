// standard headers
#include <fstream>
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

std::string ReadTextFile(const std::string &filename)
{
    std::ifstream file(filename.c_str(), std::ios::in);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
    }

    return ss.str();
}

void RunServer()
{
    const auto &server_address = std::string("localhost:50051");
    auto test_service = std::make_shared<TestServiceImpl>();

    auto creds = grpc::InsecureServerCredentials();
    auto use_ssl = true;
    if (use_ssl)
    {
        auto ssl_opts = grpc::SslServerCredentialsOptions();
        ssl_opts.pem_key_cert_pairs.push_back(
            { ReadTextFile("../../../auth/server.key"), ReadTextFile("../../../auth/server.crt") });
        creds = grpc::SslServerCredentials(ssl_opts);
    }

    ServerBuilder builder;
    builder.AddListeningPort(server_address, creds);
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
