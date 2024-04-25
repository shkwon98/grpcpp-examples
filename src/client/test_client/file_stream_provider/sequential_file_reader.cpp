// standard headers
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

// project headers
#include "sequential_file_reader.hpp"

/*=========================================================================*/

namespace
{

/**
 * @class MMapPtr
 * @brief A class for managing memory-mapped pointers.
 *
 * This class provides functionality to manage memory-mapped pointers. It takes a pointer to a memory-mapped region
 * and a length, and it will unmap the memory and close the file descriptor when the object goes out of scope.
 */
template <typename T>
class MMapPtr : public std::unique_ptr<T, std::function<void(T *)>>
{
public:
    MMapPtr(void *addr, std::size_t len, int fd = -1)
        : std::unique_ptr<T, std::function<void(T *)>>(static_cast<T *>(addr),
                                                       [len, fd](T *addr) { unmap_and_close(addr, len, fd); })
    {
        if (MAP_FAILED == addr)
        {
            throw std::system_error(std::error_code(errno, std::system_category()), "Failed to map the file into memory.");
        }

        // Inform the kernel we plan sequential access
        auto rc = posix_madvise(addr, len, POSIX_MADV_SEQUENTIAL);
        if (-1 == rc)
        {
            throw std::system_error(std::error_code(errno, std::system_category()),
                                    "Failed to set intended access pattern using posix_madvise().");
        }
    }

    MMapPtr() = delete;
    using std::unique_ptr<T, std::function<void(T *)>>::unique_ptr;
    using std::unique_ptr<T, std::function<void(T *)>>::operator=;

private:
    static void unmap_and_close(const void *addr, std::size_t len, int fd)
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

/*=========================================================================*/

SequentialFileReader::SequentialFileReader(const std::string &file)
    : file_(file)
    , data_(nullptr)
    , size_(0)
{
    if (!std::filesystem::exists(file_))
    {
        throw std::system_error(std::error_code(errno, std::system_category()), "File does not exist.");
    }
    if (!std::filesystem::is_regular_file(file_))
    {
        throw std::system_error(std::error_code(errno, std::system_category()), "Not a regular file.");
    }

    auto fd = open(file_.c_str(), O_RDONLY);
    if (fd < 0)
    {
        throw std::system_error(std::error_code(errno, std::system_category()), "Failed to open file.");
    }

    if (size_ = std::filesystem::file_size(file_); size_ > 0)
    {
        auto mmap_p = MMapPtr<const std::uint8_t>(mmap(0, size_, PROT_READ, MAP_FILE | MAP_SHARED, fd, 0), size_, -1);
        data_.swap(mmap_p);
    }
}

void SequentialFileReader::Read(std::size_t max_chunk_size)
{
    // Handle empty files. Note that data_ will likely be null, so we take care not to access it.
    if (size_ == 0)
    {
        OnChunkAvailable("", 0);
        return;
    }

    auto bytes_read = 0U;
    while (bytes_read < size_)
    {
        auto bytes_to_read = std::min(max_chunk_size, size_ - bytes_read);

        OnChunkAvailable(static_cast<const std::uint8_t *>(data_.get()) + bytes_read, bytes_to_read);
        bytes_read += bytes_to_read;
    }
}

std::string SequentialFileReader::GetFilePath() const
{
    return file_.string();
}

/*=========================================================================*/
