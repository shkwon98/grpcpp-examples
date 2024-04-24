#pragma once

#include "robl/api/service.grpc.pb.h"

#include "file_reader_into_stream.hpp"

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
    {
    }

    RegisterAccountResponse RegisterAccount(const RegisterAccountRequest &request)
    {
        grpc::ClientContext context;
        RegisterAccountResponse response;

        auto status = stub_->RegisterAccount(&context, request, &response);
        if (status.ok())
        {
            return response;
        }
        else
        {
            std::cerr << "RPC failed: " << status.error_message() << std::endl;
            return RegisterAccountResponse();
        }
    }

    void HeartBeat(void)
    {
        grpc::ClientContext context;
        std::shared_ptr<grpc::ClientReaderWriter<ClientHeartBeat, ServerHeartBeat>> stream(stub_->HeartBeat(&context));

        while (true)
        {
            ClientHeartBeat request;
            request.set_session_ix(1);
            request.set_tick(
                std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
                    .count());
            stream->Write(request);

            ServerHeartBeat response;
            if (stream->Read(&response))
            {
                std::cout << "[ServerHeartBeat] result: " << response.result() << std::endl;
                std::cout << "[ServerHeartBeat] session_ix: " << response.session_ix() << std::endl;
                std::cout << "[ServerHeartBeat] tick: " << response.tick() << std::endl;
                std::cout << "[ServerHeartBeat] om: " << response.om() << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        auto status = stream->Finish();
        if (status.ok())
        {
            std::cout << "Stream finished." << std::endl;
        }
        else
        {
            std::cerr << "Error in RPC: " << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    bool UploadFile(const std::string &filename)
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

private:
    std::unique_ptr<TestService::Stub> stub_;
};