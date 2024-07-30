#pragma once

#include <cassert>
#include <chrono>
#include <sstream>
#include <string>

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