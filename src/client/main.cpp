// standard headers
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

// grpc headers
#include <grpcpp/grpcpp.h>

// project headers
#include "test_client/test_client.hpp"

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

int main(void)
{
    // Connect to the gRPC server
    const auto &server_address = std::string("localhost:50051");

    auto creds = grpc::InsecureChannelCredentials();
    auto use_ssl = true;
    if (use_ssl)
    {
        auto ssl_opts = grpc::SslCredentialsOptions();
        ssl_opts.pem_root_certs = ReadTextFile("../../../auth/ca.crt");
        creds = grpc::SslCredentials(ssl_opts);
    }

    auto channel = grpc::CreateChannel(server_address, creds);
    auto client = TestClient(channel);

    auto threads = std::vector<std::thread>();
    // threads.emplace_back([&client]() { client.HeartBeat(); });

    auto req = 0;
    while (req != -1)
    {
        std::cin >> req;

        switch (req)
        {
        case 1: {
            RegisterAccountRequest registerRequest;
            registerRequest.set_session_id(-1);
            registerRequest.set_tick(2000);
            registerRequest.set_ip_str("192.168.0.123");
            registerRequest.set_account("admin");
            registerRequest.set_password("admin");
            registerRequest.set_realtime(System::GetSystemTickMillis());

            threads.emplace_back([&client, registerRequest]() { client.RegisterAccount(registerRequest); });
            break;
        }
        case 2:
            threads.emplace_back([&client]() { client.UploadFile("./LICENSE"); });
            break;
        case 3: {
            MarkerRequest markerRequest;
            auto mask = std::make_unique<google::protobuf::FieldMask>();
            mask->add_paths("id__");
            markerRequest.set_allocated_mask(mask.release());
            threads.emplace_back([&client, markerRequest]() { client.GetMarker(markerRequest); });
            break;
        }
        default:
            std::cout << "Invalid request" << std::endl;
            break;
        }
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    return 0;
}