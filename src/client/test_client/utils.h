#pragma once

#include <cassert>
#include <chrono>
#include <sstream>
#include <string>

// Raise a C++ system_error exception from the user-supplied error-code 'err', which
// should be a valid errno value
void raise_from_system_error_code [[noreturn]] (const std::string &user_message, int err);

// Raise a C++ system_error exception based on the current value of errno
void raise_from_errno [[noreturn]] (const std::string &user_message);

namespace System
{

long long GetSystemTickNanos();
long long GetSystemTickMillis();
long long GetSystemTickSeconds();
int GetSystemTickMinutes();

long long GetSteadyTickNanos();
long long GetSteadyTickMillis();
long long GetSteadyTickSeconds();
int GetSteadyTickMinutes();

} // namespace System

#include <iostream>
struct __TIME__CHECK__
{
    const long long begin;
    __TIME__CHECK__()
        : begin(System::GetSystemTickMillis())
    {
    }
    ~__TIME__CHECK__()
    {
        const long long end = System::GetSystemTickMillis();
        std::cout << "Elapsed Time: " << (end - begin) << std::endl;
    }
    explicit operator const bool()
    {
        return true;
    }
};
#define timecheck if (__TIME__CHECK__ _TIME_CHECK_RUN_{})

#define KB (1ULL << 10)
#define MB (1ULL << 20)
#define GB (1ULL << 30)