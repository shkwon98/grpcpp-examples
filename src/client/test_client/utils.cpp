#include "utils.h"

void raise_from_system_error_code(const std::string &user_message, int err)
{
    std::ostringstream sts;
    if (user_message.size() > 0)
    {
        sts << user_message << ' ';
    }

    assert(0 != err);
    throw std::system_error(std::error_code(err, std::system_category()), sts.str().c_str());
}

void raise_from_errno(const std::string &user_message)
{
    raise_from_system_error_code(user_message, errno);
}

namespace System
{

long long GetSystemTickNanos()
{
    auto time = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
}

long long GetSystemTickMillis()
{
    auto time = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

long long GetSystemTickSeconds()
{
    auto time = std::chrono::system_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
}

int GetSystemTickMinutes()
{
    auto time = std::chrono::system_clock::now();
    return (int)(std::chrono::duration_cast<std::chrono::minutes>(time.time_since_epoch()).count());
}

long long GetSteadyTickNanos()
{
    auto time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(time.time_since_epoch()).count();
}

long long GetSteadyTickMillis()
{
    auto time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(time.time_since_epoch()).count();
}

long long GetSteadyTickSeconds()
{
    auto time = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
}

int GetSteadyTickMinutes()
{
    auto time = std::chrono::steady_clock::now();
    return (int)(std::chrono::duration_cast<std::chrono::minutes>(time.time_since_epoch()).count());
}

} // namespace System