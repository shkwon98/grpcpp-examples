#pragma once

// standard headers
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

/*=========================================================================*/

class SequentialFileWriter
{
public:
    SequentialFileWriter()
        : no_space_(false)
    {
    }
    SequentialFileWriter(SequentialFileWriter &&) = default;
    SequentialFileWriter &operator=(SequentialFileWriter &&) = default;

    /**
     * Opens the file if it is not already open. If the file is already open, this method does nothing.
     * On errors, this method throws an exception derived from std::system_error.
     *
     * @param name The path to the file to be opened.
     */
    void OpenIfNecessary(const std::filesystem::path &name)
    {
        if (ofs_.is_open())
        {
            return;
        }

        std::ofstream ofs;
        ofs.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try
        {
            std::filesystem::create_directories(name.parent_path());
            ofs.open(name, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        }
        catch (const std::system_error &ex)
        {
            RaiseError("opening", ex);
        }

        ofs_ = std::move(ofs);
        name_ = name;
        no_space_ = false;
        permission_error_ = false;
        return;
    }

    /**
     * Write data from a string. On errors throws an exception derived from std::system_error. This method may take ownership
     * of the string. Hence no assumption may be made about the data it contains after it returns.
     *
     * @param data The data to be written to the file.
     */
    void Write(std::string &data)
    {
        try
        {
            ofs_ << data;
        }
        catch (const std::system_error &ex)
        {
            if (ofs_.is_open())
            {
                ofs_.close();
            }
            std::remove(name_.c_str()); // Best effort. We expect it to succeed, but we don't check whether it did
            RaiseError("writing to", ex);
        }

        data.clear();
        return;
    }

    /**
     * Checks if there is no space left for writing.
     *
     * @return true if there is no space left, false otherwise.
     */
    bool NoSpaceLeft() const
    {
        return no_space_;
    }

private:
    /**
     * Raises an error with the specified action attempted and system error.
     *
     * @param action_attempted The action that was attempted when the error occurred.
     * @param err The system error that occurred.
     */
    void RaiseError(const std::string action_attempted, const std::system_error &err)
    {
        const auto ec = err.code().value();

        switch (ec)
        {
        case ENOSPC:
        case EFBIG:
            no_space_ = true;
            break;
        case EACCES:
        case EPERM:
        case EROFS:
            permission_error_ = true;
            break;
        default:
            break;
        }

        std::ostringstream sts;
        sts << "Error " << action_attempted << " the file " << name_ << ": ";

        assert(0 != ec);
        throw std::system_error(std::error_code(ec, std::system_category()), sts.str().c_str());
    }

    std::string name_;
    std::ofstream ofs_;
    bool no_space_;
    bool permission_error_;
};

/*=========================================================================*/
