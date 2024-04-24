#pragma once

// standard headers
#include <iostream>

// grpc headers
#include <robl/api/service.grpc.pb.h>

// project headers
#include "file_reader_into_stream.hpp"
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
    void HeartBeat(void);
    bool UploadFile(const std::string &filename);

private:
    std::unique_ptr<TestService::Stub> stub_;
    uint32_t session_id_;
};

inline RegisterAccountResponse TestClient::RegisterAccount(const RegisterAccountRequest &request)
{
    grpc::ClientContext context;
    RegisterAccountResponse response;

    auto status = stub_->RegisterAccount(&context, request, &response);
    if (!status.ok())
    {
        std::cerr << "RPC failed: " << status.error_message() << std::endl;
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

inline void TestClient::HeartBeat(void)
{
    grpc::ClientContext context;
    std::shared_ptr<grpc::ClientReaderWriter<ClientHeartBeat, ServerHeartBeat>> streamer(stub_->HeartBeat(&context));

    while (true)
    {
        ClientHeartBeat request;
        request.set_session_id(session_id_);
        request.set_tick(std::chrono::system_clock::now().time_since_epoch().count());
        request.set_tick(System::GetSystemTickMillis());
        streamer->Write(request);

        ServerHeartBeat response;
        if (streamer->Read(&response))
        {
            std::cout << "[ServerHeartBeat] result: " << response.result() << std::endl;
            std::cout << "[ServerHeartBeat] session_id: " << response.session_id() << std::endl;
            std::cout << "[ServerHeartBeat] tick: " << response.tick() << std::endl;
            std::cout << "[ServerHeartBeat] om: " << response.om() << std::endl;
        }
        std::this_thread::sleep_for(1s);
    }

    auto status = streamer->Finish();
    if (status.ok())
    {
        std::cout << "Stream finished." << std::endl;
    }
    else
    {
        std::cerr << "Error in RPC: " << status.error_code() << ": " << status.error_message() << std::endl;
    }
}

inline bool TestClient::UploadFile(const std::string &filename)
{
    grpc::ClientContext context;

    std::unique_ptr<grpc::ClientReaderWriter<FileContent, Status>> writer(stub_->UploadFile(&context));
    try
    {
        FileReaderIntoStream<grpc::ClientReaderWriter<FileContent, Status>> reader(filename, *writer);

        const size_t chunk_size = 1UL << 20; // Hardcoded to 1MB, which seems to be recommended from experience.
        reader.Read(chunk_size);
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Failed to send the file " << filename << ": " << ex.what() << std::endl;
    }

    writer->WritesDone();
    grpc::Status status = writer->Finish();
    if (!status.ok())
    {
        std::cerr << "File Uploading rpc failed: " << status.error_message() << std::endl;
        return false;
    }
    else
    {
        std::cout << "Finished sending file" << std::endl;
    }

    return true;
}
