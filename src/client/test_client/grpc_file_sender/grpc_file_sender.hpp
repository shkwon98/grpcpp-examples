#pragma once

// standard headers
#include <filesystem>
#include <string>

// grpc headers
#include <robl/api/test.pb.h>

// project headers
#include "sequential_file_reader.h"

template <class GrpcWriter>
class GrpcFileSender : public SequentialFileReader
{
public:
    GrpcFileSender(const std::string &filename, GrpcWriter &writer)
        : SequentialFileReader(filename)
        , writer_(writer)
    {
    }

protected:
    virtual void OnChunkAvailable(const void *data, std::size_t size) override
    {
        robl::api::FileContent fc;

        fc.set_name(std::filesystem::path(GetFilePath()).filename());
        fc.set_content(data, size);

        if (!writer_.Write(fc))
        {
            throw std::system_error(std::make_error_code(std::errc::connection_aborted),
                                    "The server aborted the connection.");
        }
    }

private:
    GrpcWriter &writer_;
};