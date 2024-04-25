#pragma once

// standard headers
#include <filesystem>
#include <functional>

/**
 * @class SequentialFileReader
 * @brief A class for reading files sequentially.
 *
 * This class provides functionality to read files sequentially. It takes a file name as input
 * and allows reading the file in a sequential manner.
 */
class SequentialFileReader
{
public:
    SequentialFileReader(SequentialFileReader &&) = default;
    SequentialFileReader &operator=(SequentialFileReader &&) = default;
    ~SequentialFileReader() = default;

    /**
     * Reads data from the file, calling OnChunkAvailable() method whenever each data chunk is available. It blocks until the
     * reading is complete.
     * @param max_chunk_size The maximum size of each chunk to read.
     */
    void Read(std::size_t max_chunk_size);

    /**
     * Returns the file path.
     *
     * @return The file path.
     */
    std::string GetFilePath() const;

protected:
    /**
     * Attempts to open the file for reading and, if successful, it will read the file size and then
     * memory-map the file for sequential access using mmap(). It throws std::system_error if it fails to do so.
     *
     * @param file The file path to read.
     */
    SequentialFileReader(const std::string &file);

    /**
     * The user needs to override this function to get called when a chunk of data is available.
     *
     * @param data A pointer to the chunk of data.
     * @param size The size of the chunk of data.
     */
    virtual void OnChunkAvailable(const void *data, std::size_t size) = 0;

private:
    std::filesystem::path file_;
    std::unique_ptr<const std::uint8_t, std::function<void(const std::uint8_t *)>> data_;
    std::size_t size_;
};