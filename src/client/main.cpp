// standard headers
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

// grpc++ headers
#include <grpcpp/grpcpp.h>

// project headers
#include "test_client/test_client.hpp"

int main()
{
    // Connect to the gRPC server
    const auto server_address = "localhost:50051";
    auto client = TestClient(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

    auto threads = std::vector<std::thread>();
    threads.emplace_back([&client]() {
        for (auto i = 0; i < 10; ++i)
        {
            RegisterAccountRequest registerRequest;
            registerRequest.set_session_ix(-1);
            registerRequest.set_tick(2000);
            registerRequest.set_ip_str("192.168.0.123");
            registerRequest.set_account("admin");
            registerRequest.set_password("admin");
            registerRequest.set_realtime(
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count());

            RegisterAccountResponse registerResponse = client.RegisterAccount(registerRequest);
            std::cout << "[RegisterAccountResponse] result: " << registerResponse.result() << std::endl;
            std::cout << "[RegisterAccountResponse] session_ix: " << registerResponse.session_ix() << std::endl;
            std::cout << "[RegisterAccountResponse] tick: " << registerResponse.tick() << std::endl;
            std::cout << "[RegisterAccountResponse] account_id: " << registerResponse.account_id() << std::endl;
            std::cout << "[RegisterAccountResponse] account_pri: " << registerResponse.account_pri() << std::endl;
            std::cout << "[RegisterAccountResponse] ip: " << registerResponse.ip() << std::endl;
        }
    });
    threads.emplace_back([&client]() { client.HeartBeat(); });
    threads.emplace_back([&client]() { client.UploadFile("./LICENSE"); });

    for (auto &thread : threads)
    {
        thread.join();
    }

    return 0;
}