#include "utils.h"

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
    return static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(time.time_since_epoch()).count());
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
    return static_cast<int>(std::chrono::duration_cast<std::chrono::minutes>(time.time_since_epoch()).count());
}

} // namespace System