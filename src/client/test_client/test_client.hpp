#pragma once

// standard headers
#include <iostream>

// grpc headers
#include <robl/api/service.grpc.pb.h>

// project headers
#include "file_stream_provider/file_stream_provider.hpp"
#include "utils.h"

using namespace std::chrono_literals;

using robl::api::ClientHeartBeat;
using robl::api::FileContent;
using robl::api::RegisterAccountRequest;
using robl::api::RegisterAccountResponse;
using robl::api::ServerHeartBeat;
using robl::api::Status;
using robl::api::TestService;

class TestClient
{
public:
    explicit TestClient(std::shared_ptr<grpc::Channel> channel)
        : stub_(TestService::NewStub(channel))
        , session_id_(-1)
    {
    }

    RegisterAccountResponse RegisterAccount(const RegisterAccountRequest &request);
    bool HeartBeat(void);
    bool UploadFile(const std::string &filename);

private:
    std::unique_ptr<TestService::Stub> stub_;
    uint32_t session_id_;
};

inline RegisterAccountResponse TestClient::RegisterAccount(const RegisterAccountRequest &request)
{
    grpc::ClientContext context;
    RegisterAccountResponse response;

    const auto status = stub_->RegisterAccount(&context, request, &response);

    if (!status.ok())
    {
        std::cerr << "RegisterAccount rpc failed: " << status.error_code() << ": " << status.error_message() << std::endl;
        return RegisterAccountResponse();
    }

    std::cout << "[RegisterAccountResponse] result: " << response.result() << std::endl
              << "[RegisterAccountResponse] session_id: " << response.session_id() << std::endl
              << "[RegisterAccountResponse] tick: " << response.tick() << std::endl
              << "[RegisterAccountResponse] account_id: " << response.account_id() << std::endl
              << "[RegisterAccountResponse] account_pri: " << response.account_pri() << std::endl
              << "[RegisterAccountResponse] ip: " << response.ip() << std::endl;

    session_id_ = response.session_id();
    return response;
}

inline bool TestClient::HeartBeat(void)
{
    grpc::ClientContext context;
    std::shared_ptr<grpc::ClientReaderWriter<ClientHeartBeat, ServerHeartBeat>> streamer(stub_->HeartBeat(&context));
    ClientHeartBeat request;
    ServerHeartBeat response;

    while (true)
    {
        const auto &now = std::chrono::system_clock::now();

        request.set_session_id(session_id_);
        request.set_tick(System::GetSystemTickMillis());
        streamer->Write(request);

        if (streamer->Read(&response))
        {
            std::cout << "[ServerHeartBeat] result: " << response.result() << std::endl
                      << "[ServerHeartBeat] session_id: " << response.session_id() << std::endl
                      << "[ServerHeartBeat] tick: " << response.tick() << std::endl
                      << "[ServerHeartBeat] om: " << response.om() << std::endl;
        }

        std::this_thread::sleep_until(now + 1s);
    }

    const auto status = streamer->Finish();

    if (!status.ok())
    {
        std::cerr << "HeartBeat rpc failed: " << status.error_code() << ": " << status.error_message() << std::endl;
        return false;
    }

    return true;
}

inline bool TestClient::UploadFile(const std::string &filename)
{
    grpc::ClientContext context;
    std::unique_ptr<grpc::ClientReaderWriter<FileContent, Status>> streamer(stub_->UploadFile(&context));

    try
    {
        auto reader = FileStreamProvider(filename, *streamer);

        const size_t chunk_size = 1 * KB; // Hardcoded to 1MB, which seems to be recommended from experience.
        reader.Read(chunk_size);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Failed to send the file " << filename << ": " << ex.what() << std::endl;
    }

    streamer->WritesDone();
    const auto status = streamer->Finish();

    if (!status.ok())
    {
        std::cerr << "UploadFile rpc failed: " << status.error_code() << ": " << status.error_message() << std::endl;
        return false;
    }

    return true;
}
