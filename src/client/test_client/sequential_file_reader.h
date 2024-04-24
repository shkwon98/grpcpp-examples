#pragma once

#include <fcntl.h>
#include <filesystem>
#include <sys/mman.h>
#include <sys/stat.h>

#include "utils.h"

namespace
{

using VoidPtr = std::unique_ptr<void, std::function<void(void *)>>;

class MMapPtr : public VoidPtr
{
public:
    MMapPtr(size_t size, int fd)
        : VoidPtr(mapping_, [size, fd](void *addr) { UnmapAndClose(addr, size, fd); })
        , mapping_(mmap(0, size, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0))
    {
        if (mapping_ == MAP_FAILED)
        {
            throw std::system_error(std::error_code(errno, std::system_category()), "Failed to map the file into memory.");
        }

        // Inform the kernel we plan sequential access
        int rc = posix_madvise(mapping_, size, POSIX_MADV_SEQUENTIAL);
        if (rc == -1)
        {
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Failed to set intended access pattern using posix_madvise().");
        }
    }
    MMapPtr() = delete;
    using VoidPtr::unique_ptr;
    using VoidPtr::operator=;

private:
    static void UnmapAndClose(const void *addr, size_t len, int fd)
    {
        if ((MAP_FAILED != addr) && (nullptr != addr) && (len > 0))
        {
            munmap(const_cast<void *>(addr), len);
        }

        if (fd >= 0)
        {
            close(fd);
        }

        return;
    }

    void *mapping_;
};

}; // Anonymous namespace

// SequentialFileReader: Read a file using using mmap(). Attempt to overlap reads of the file and writes by the user's code
// by reading the next segment

class SequentialFileReader
{
public:
    SequentialFileReader(SequentialFileReader &&) = default;
    SequentialFileReader &operator=(SequentialFileReader &&) = default;
    ~SequentialFileReader() = default;

    // Read the file, calling OnChunkAvailable() whenever data are available. It blocks until the reading
    // is complete.
    void Read(size_t max_chunk_size)
    {
        size_t bytes_read = 0;

        // Handle empty files. Note that data_ will likely be null, so we take care not to access it.
        if (0 == size_)
        {
            OnChunkAvailable("", 0);
            return;
        }

        while (bytes_read < size_)
        {
            size_t bytes_to_read = std::min(max_chunk_size, size_ - bytes_read);

            OnChunkAvailable(static_cast<const uint8_t *>(data_.get()) + bytes_read, bytes_to_read);

            bytes_read += bytes_to_read;
        }
    }

    std::string GetFilePath() const
    {
        return file_path_;
    }

protected:
    // Constructor. Attempts to open the file, and throws std::system_error if it fails to do so.
    SequentialFileReader(const std::string &file_name)
        : file_path_(file_name)
        , data_(nullptr)
        , size_(0)
    {
        if (!std::filesystem::exists(file_name))
        {
            throw std::system_error(std::error_code(errno, std::system_category()), "File does not exist.");
        }
        if (!std::filesystem::is_regular_file(file_name))
        {
            throw std::system_error(std::error_code(errno, std::system_category()), "Not a regular file.");
        }

        auto fd = open(file_name.c_str(), O_RDONLY);
        if (fd < 0)
        {
            throw std::system_error(std::error_code(errno, std::system_category()), "Failed to open file.");
        }

        if (size_ = std::filesystem::file_size(file_name); size_ > 0)
        {
            auto mmap_p = MMapPtr(size_, fd);

            data_.swap(mmap_p);
        }
    }

    // OnChunkAvailable: The user needs to override this function to get called when data become available.
    virtual void OnChunkAvailable(const void *data, size_t size) = 0;

private:
    std::string file_path_;
    std::unique_ptr<void, std::function<void(void *)>> data_;
    size_t size_;
};