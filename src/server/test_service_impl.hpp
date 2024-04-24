#pragma once

#include <arpa/inet.h>
#include <filesystem>
#include <thread>

#include "robl/api/service.grpc.pb.h"

#include "sequential_file_writer.h"

using robl::api::ClientHeartBeat;
using robl::api::FileContent;
using robl::api::RegisterAccountRequest;
using robl::api::RegisterAccountResponse;
using robl::api::ServerHeartBeat;
using robl::api::Status;
using robl::api::TestService;

class TestServiceImpl final : public TestService::Service
{
public:
    TestServiceImpl(void)
        : root_path_(std::filesystem::current_path() / "uploads")
    {
    }
    ::grpc::Status RegisterAccount(::grpc::ServerContext *context, const RegisterAccountRequest *request,
                                   RegisterAccountResponse *response) override
    {
        if (request->session_id() != -1)
        {
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "session_id is not -1");
        }

        response->set_result(0);
        response->set_session_id(1);
        response->set_tick(request->tick());
        if (request->account() == "admin")
        {
            response->set_account_id(65534);
            response->set_account_pri(65534);
        }
        else
        {
            response->set_account_id(1);
            response->set_account_pri(1);
        }
        response->set_ip(inet_addr(request->ip_str().c_str()));

        return ::grpc::Status::OK;
    }

    ::grpc::Status HeartBeat(::grpc::ServerContext *context,
                             ::grpc::ServerReaderWriter<ServerHeartBeat, ClientHeartBeat> *stream) override
    {
        ServerHeartBeat serverHeartBeat;
        ClientHeartBeat clientHeartBeat;
        while (stream->Read(&clientHeartBeat))
        {
            std::cout << "[HeartBeat] session_id: " << clientHeartBeat.session_id() << std::endl
                      << "[HeartBeat] tick: " << clientHeartBeat.tick() << std::endl;

            serverHeartBeat.set_result(0);
            serverHeartBeat.set_session_id(clientHeartBeat.session_id());
            serverHeartBeat.set_tick(clientHeartBeat.tick());
            serverHeartBeat.set_om(11);

            stream->Write(serverHeartBeat);
        }

        std::cout << "[HeartBeat] stream closed" << std::endl;
        return ::grpc::Status::OK;
    }

    ::grpc::Status UploadFile(::grpc::ServerContext *context,
                              ::grpc::ServerReaderWriter<Status, FileContent> *stream) override
    {
        FileContent content_part;
        SequentialFileWriter writer;

        while (stream->Read(&content_part))
        {
            std::cout << "[UploadFile] name: " << content_part.name() << std::endl
                      << "[UploadFile] content size: " << content_part.content().size() << std::endl;

            try
            {
                writer.OpenIfNecessary(root_path_ / content_part.name());
                auto *const data = content_part.mutable_content();
                writer.Write(*data);
            }
            catch (const std::system_error &ex)
            {
                const auto status_code =
                    (writer.NoSpaceLeft() ? ::grpc::StatusCode::RESOURCE_EXHAUSTED : ::grpc::StatusCode::ABORTED);
                return ::grpc::Status(status_code, ex.what());
            }
        }

        return ::grpc::Status::OK;
    }

private:
    std::filesystem::path root_path_;
};
