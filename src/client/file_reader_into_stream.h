#pragma once

#include <cstdint>
#include <string>
#include <sys/errno.h>

#include "sequential_file_reader.h"
#include "utils.h"

template <class StreamWriter>
class FileReaderIntoStream : public SequentialFileReader
{
public:
    FileReaderIntoStream(const std::string &filename, StreamWriter &writer)
        : SequentialFileReader(filename)
        , writer_(writer)
    {
    }

    // using SequentialFileReader::SequentialFileReader;
    // using SequentialFileReader::operator=;

protected:
    virtual void OnChunkAvailable(const void *data, size_t size) override
    {
        const std::string remote_filename = extract_basename(GetFilePath());

        robl::api::FileContent fc;
        fc.set_name(std::move(remote_filename));
        fc.set_content(data, size);

        if (!writer_.Write(fc))
        {
            raise_from_system_error_code("The server aborted the connection.", ECONNRESET);
        }
    }

private:
    StreamWriter &writer_;
};