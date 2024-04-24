#pragma once

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

class SequentialFileWriter
{
public:
    SequentialFileWriter()
        : no_space_(false)
    {
    }
    SequentialFileWriter(SequentialFileWriter &&) = default;
    SequentialFileWriter &operator=(SequentialFileWriter &&) = default;

    // Open the file at the relative path 'name' for writing. On errors throw std::system_error
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

    // Write data from a string. On errors throws an exception drived from std::system_error
    // This method may take ownership of the string. Hence no assumption may be made about
    // the data it contains after it returns.
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

    bool NoSpaceLeft() const
    {
        return no_space_;
    }

private:
    void RaiseError(const std::string action_attempted, const std::system_error &ex)
    {
        const auto ec = ex.code().value();

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

        assert(0 != err);
        throw std::system_error(std::error_code(ec, std::system_category()), sts.str().c_str());
    }

    std::string name_;
    std::ofstream ofs_;
    bool no_space_;
    bool permission_error_;
};