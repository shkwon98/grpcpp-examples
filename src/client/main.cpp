// standard headers
#include <chrono>
#include <string>
#include <thread>
#include <vector>

// grpc headers
#include <grpcpp/grpcpp.h>

// project headers
#include "test_client/test_client.hpp"

int main()
{
    // Connect to the gRPC server
    const auto server_address = "localhost:50051";
    auto client = TestClient(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    for (auto i = 0; i < 10; ++i)
    {
        RegisterAccountRequest registerRequest;
        registerRequest.set_session_id(-1);
        registerRequest.set_tick(2000);
        registerRequest.set_ip_str("192.168.0.123");
        registerRequest.set_account("admin");
        registerRequest.set_password("admin");
        registerRequest.set_realtime(System::GetSystemTickMillis());

        client.RegisterAccount(registerRequest);
    }

    auto threads = std::vector<std::thread>();
    threads.emplace_back([&client]() { client.HeartBeat(); });
    threads.emplace_back([&client]() { client.UploadFile("./LICENSE"); });

    for (auto &thread : threads)
    {
        thread.join();
    }

    return 0;
}