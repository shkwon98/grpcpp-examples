#pragma once

// standard headers
#include <arpa/inet.h>
#include <filesystem>
#include <thread>

// grpc headers
#include <google/protobuf/util/field_mask_util.h>
#include <robl/api/service.grpc.pb.h>

// project headers
#include "sequential_file_writer.h"

/*=========================================================================*/

using robl::api::ClientHeartBeat;
using robl::api::FileContent;
using robl::api::MarkerInfo;
using robl::api::MarkerRequest;
using robl::api::MarkerResponse;
using robl::api::RegisterAccountRequest;
using robl::api::RegisterAccountResponse;
using robl::api::ServerHeartBeat;
using robl::api::Status;
using robl::api::TestService;

/*=========================================================================*/

class TestServiceImpl final : public TestService::Service
{
public:
    TestServiceImpl(void)
        : root_path_(std::filesystem::current_path() / "uploads")
    {
    }

    // TestService rpc methods
    grpc::Status RegisterAccount(grpc::ServerContext *context, const RegisterAccountRequest *request,
                                 RegisterAccountResponse *response) override;
    grpc::Status HeartBeat(grpc::ServerContext *context,
                           grpc::ServerReaderWriter<ServerHeartBeat, ClientHeartBeat> *stream) override;
    grpc::Status UploadFile(grpc::ServerContext *context, grpc::ServerReaderWriter<Status, FileContent> *stream) override;
    grpc::Status GetMarker(grpc::ServerContext *context, const MarkerRequest *request, MarkerResponse *response) override;

private:
    std::filesystem::path root_path_;
};

/*=========================================================================*/

inline grpc::Status TestServiceImpl::RegisterAccount(grpc::ServerContext *context, const RegisterAccountRequest *request,
                                                     RegisterAccountResponse *response)
{
    if (request->session_id() != -1)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "session_id is not -1");
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

    return grpc::Status::OK;
}

inline grpc::Status TestServiceImpl::HeartBeat(grpc::ServerContext *context,
                                               grpc::ServerReaderWriter<ServerHeartBeat, ClientHeartBeat> *stream)
{
    ServerHeartBeat serverHeartBeat;
    ClientHeartBeat clientHeartBeat;

    while (stream->Read(&clientHeartBeat))
    {
        // std::cout << "[HeartBeat] session_id: " << clientHeartBeat.session_id() << std::endl
        //           << "[HeartBeat] tick: " << clientHeartBeat.tick() << std::endl;

        // std::cout << clientHeartBeat.DebugString() << std::endl;
        std::cout << clientHeartBeat.ShortDebugString() << std::endl;

        serverHeartBeat.set_result(0);
        serverHeartBeat.set_session_id(clientHeartBeat.session_id());
        serverHeartBeat.set_tick(clientHeartBeat.tick());
        serverHeartBeat.set_om(11);

        stream->Write(serverHeartBeat);
    }

    std::cout << "[HeartBeat] stream closed" << std::endl;
    return grpc::Status::OK;
}

inline grpc::Status TestServiceImpl::UploadFile(grpc::ServerContext *context,
                                                grpc::ServerReaderWriter<Status, FileContent> *stream)
{
    FileContent content_part;
    SequentialFileWriter file_writer;

    while (stream->Read(&content_part))
    {
        std::cout << "[UploadFile] name: " << content_part.name() << std::endl
                  << "[UploadFile] content size: " << content_part.content().size() << std::endl;

        try
        {
            file_writer.OpenIfNecessary(root_path_ / content_part.name());
            auto *const data = content_part.mutable_content();
            const auto size = data->size();
            file_writer.Write(*data);

            std::stringstream ss;
            ss << "Uploaded " << size << " bytes.";

            Status status;
            status.set_code(0);
            status.set_message(ss.str());
            stream->Write(status);
        }
        catch (const std::system_error &ex)
        {
            const auto status_code =
                (file_writer.NoSpaceLeft() ? grpc::StatusCode::RESOURCE_EXHAUSTED : grpc::StatusCode::ABORTED);
            return grpc::Status(status_code, ex.what());
        }
    }

    return grpc::Status::OK;
}

inline grpc::Status TestServiceImpl::GetMarker(grpc::ServerContext *context, const MarkerRequest *request,
                                               MarkerResponse *response)
{
    auto id = request->id();
    if (!google::protobuf::util::FieldMaskUtil::IsValidFieldMask<MarkerInfo>(request->mask()))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid field mask");
    }

    auto field_mask_1 = google::protobuf::FieldMask();
    auto field_mask_2 = google::protobuf::FieldMask();
    auto field_mask_3 = google::protobuf::FieldMask();

    std::cout << ">>> 1: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_1) << std::endl;

    google::protobuf::util::FieldMaskUtil::AddPathToFieldMask<MarkerInfo>("total_count", &field_mask_1);
    google::protobuf::util::FieldMaskUtil::AddPathToFieldMask<MarkerInfo>("markers", &field_mask_1);
    std::cout << ">>> 2: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_1) << std::endl;

    google::protobuf::util::FieldMaskUtil::FromFieldNumbers<MarkerInfo>({ 1, 2 }, &field_mask_2);
    std::cout << ">>> 3: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_2) << std::endl;

    google::protobuf::util::FieldMaskUtil::ToCanonicalForm(field_mask_1, &field_mask_3);
    std::cout << ">>> 4: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_3) << std::endl;

    google::protobuf::util::FieldMaskUtil::GetFieldMaskForAllFields<MarkerInfo>(&field_mask_1);
    std::cout << ">>> 5: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_1) << std::endl;

    google::protobuf::util::FieldMaskUtil::Intersect(field_mask_1, field_mask_2, &field_mask_3);
    std::cout << ">>> 6: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_3) << std::endl;

    google::protobuf::util::FieldMaskUtil::Subtract<MarkerInfo>(field_mask_1, field_mask_2, &field_mask_3);
    std::cout << ">>> 7: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_3) << std::endl;

    google::protobuf::util::FieldMaskUtil::Union(field_mask_1, field_mask_2, &field_mask_3);
    std::cout << ">>> 8: " << google::protobuf::util::FieldMaskUtil::ToString(field_mask_3) << std::endl;

    std::cout << "IsPathInFieldMask(\"id\", field_mask_1): " << std::boolalpha
              << google::protobuf::util::FieldMaskUtil::IsPathInFieldMask("id", field_mask_1) << std::endl;

    std::cout << "IsValidPath<MarkerInfo>(\"id\"): " << std::boolalpha
              << google::protobuf::util::FieldMaskUtil::IsValidPath<MarkerInfo>("id") << std::endl;

    MarkerInfo marker_info;
    marker_info.mutable_markers(1)->set_id(1);
    marker_info.mutable_markers(1)->mutable_coordinate()->set_latitude(37.7749);
    marker_info.mutable_markers(1)->mutable_coordinate()->set_longitude(-122.4194);
    marker_info.mutable_markers(1)->set_radius(100.0);
    marker_info.mutable_markers(1)->set_description("This is a marker");

    response->mutable_marker_info()->mutable_markers(1)->set_name("marker");

    auto options = google::protobuf::util::FieldMaskUtil::MergeOptions();
    options.set_replace_message_fields(true);
    options.set_replace_repeated_fields(true);

    google::protobuf::util::FieldMaskUtil::MergeMessageTo(marker_info, field_mask_1, options,
                                                          response->mutable_marker_info());
    std::cout << response->DebugString() << std::endl;

    google::protobuf::util::FieldMaskUtil::TrimMessage(field_mask_2, response->mutable_marker_info());
    std::cout << response->DebugString() << std::endl;

    // std::cout << "[GetMarker] id: " << id << std::endl << "[GetMarker] mask: " << field_mask_1 << std::endl;

    // auto *marker = response->mutable_marker();

    // marker->set_id(id);
    // marker->set_name("marker");
    // marker->mutable_coordinate()->set_latitude(37.7749);
    // marker->mutable_coordinate()->set_longitude(-122.4194);
    // marker->set_radius(100.0);
    // marker->set_description("This is a marker");

    return grpc::Status::OK;
}
