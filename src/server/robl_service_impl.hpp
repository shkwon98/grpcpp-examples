#pragma once

#include <google/protobuf/util/time_util.h>

#include "robl/api/service.grpc.pb.h"

using google::protobuf::util::TimeUtil;

class RoblService
{
protected:
    bool CheckHeader(const ::robl::api::RequestHeader &header) const
    {
        if (header.client_name() == "admin")
        {
            return true;
        }

        return false;
    }

    template <typename RequestType>
    void InitializeResponseHeader(::robl::api::ResponseHeader *response_header, const RequestType *request)
    {
        const auto &request_header = request->header();
        const auto &timestamp = TimeUtil::GetCurrentTime();

        response_header->mutable_request_received_timestamp()->CopyFrom(timestamp);
        response_header->mutable_request_header()->CopyFrom(request_header);
        response_header->mutable_request()->PackFrom(*request);
    }

    void CompleteResponseHeader(::robl::api::ResponseHeader *response_header) const
    {
        const auto &timestamp = TimeUtil::GetCurrentTime();

        response_header->mutable_response_timestamp()->CopyFrom(timestamp);
    }
};
