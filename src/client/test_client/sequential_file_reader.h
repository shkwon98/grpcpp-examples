#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "utils.h"

namespace
{

template <typename T>
class MMapPtr : public std::unique_ptr<T, std::function<void(T *)>>
{
public:
    MMapPtr(T *addr, size_t len, int fd = -1)
        : std::unique_ptr<T, std::function<void(T *)>>(addr, [len, fd](T *addr) { unmap_and_close(addr, len, fd); })
    {
    }

    MMapPtr()
        : MMapPtr(nullptr, 0, -1)
    {
    }

    using std::unique_ptr<T, std::function<void(T *)>>::unique_ptr;
    using std::unique_ptr<T, std::function<void(T *)>>::operator=;

private:
    static void unmap_and_close(const void *addr, size_t len, int fd)
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

            OnChunkAvailable(data_.get() + bytes_read, bytes_to_read);

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
        int fd = open(file_name.c_str(), O_RDONLY);
        if (-1 == fd)
        {
            raise_from_errno("Failed to open file.");
        }

        // Ensure that fd will be closed if this method aborts at any point
        MMapPtr<const std::uint8_t> mmap_p(nullptr, 0, fd);

        struct stat st
        {
        };
        int rc = fstat(fd, &st);
        if (-1 == rc)
        {
            raise_from_errno("Failed to read file size.");
        }
        size_ = st.st_size;
        if (size_ > 0)
        {
            // std::cout << size_ << ' ' << PROT_READ << ' ' << MAP_FILE << ' ' << fd << std::endl;
            void *const mapping = mmap(0, size_, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
            if (MAP_FAILED == mapping)
            {
                raise_from_errno("Failed to map the file into memory.");
            }

            // Close the file descriptor, and protect the newly acquired memory mapping inside an object
            mmap_p = MMapPtr<const std::uint8_t>(static_cast<std::uint8_t *>(mapping), size_, -1);
            // Inform the kernel we plan sequential access
            rc = posix_madvise(mapping, size_, POSIX_MADV_SEQUENTIAL);
            if (-1 == rc)
            {
                raise_from_errno("Failed to set intended access pattern useing posix_madvise().");
            }

            data_.swap(mmap_p);
        }
    }

    // TODO: Also provide a constructor that doesn't open the file, and a separate Open method.

    // OnChunkAvailable: The user needs to override this function to get called when data become available.
    virtual void OnChunkAvailable(const void *data, size_t size) = 0;

private:
    std::string file_path_;
    std::unique_ptr<const std::uint8_t, std::function<void(const std::uint8_t *)>> data_;
    size_t size_;
};