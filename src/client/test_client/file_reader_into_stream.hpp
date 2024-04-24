#pragma once

#include <filesystem>
#include <string>

#include "sequential_file_reader.h"

template <class GrpcWriter>
class FileReaderIntoStream : public SequentialFileReader
{
public:
    FileReaderIntoStream(const std::string &filename, GrpcWriter &writer)
        : SequentialFileReader(filename)
        , writer_(writer)
    {
    }

protected:
    virtual void OnChunkAvailable(const void *data, size_t size) override
    {
        const std::string remote_filename = std::filesystem::path(GetFilePath()).filename();

        robl::api::FileContent fc;
        fc.set_name(std::move(remote_filename));
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