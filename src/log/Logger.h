#pragma once

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>

// 컴파일 타임 최소 로그 레벨. 미지정 시 0(Debug)=전부 출력.
#ifndef GS_LOG_MIN_LEVEL
#define GS_LOG_MIN_LEVEL 0
#endif

namespace gs
{

enum class LogLevel
{
    Debug,
    Info,
    Warn,
    Error
};

inline const char* ToString(LogLevel in_level)
{
    switch (in_level)
    {
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO ";
        case LogLevel::Warn:
            return "WARN ";
        case LogLevel::Error:
            return "ERROR";
    }
    return "?????";
}

class Logger
{
public:
    static Logger& Instance()
    {
        static Logger m_instance;
        return m_instance;
    }

    void Log(LogLevel in_level, const std::string& in_msg)
    {
        using namespace std::chrono;
        const auto now  = system_clock::now();
        const auto time = system_clock::to_time_t(now);
        const auto ms   = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

        std::tm tm{};
#ifdef _WIN32
        localtime_s(&tm, &time);
#else
        localtime_r(&time, &tm);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm, "%H:%M:%S")
            << '.' << std::setw(3) << std::setfill('0') << ms.count()
            << " [" << ToString(in_level) << "] " << in_msg;

        std::lock_guard<std::mutex> lock(m_mutex);
        std::cout << oss.str() << std::endl;
    }

private:
    Logger() = default;
    std::mutex m_mutex;
};

} // namespace gs

#define LOG_DEBUG(msg) do { if (0 >= GS_LOG_MIN_LEVEL) \
::gs::Logger::Instance().Log(::gs::LogLevel::Debug, (msg)); } while (0)
#define LOG_INFO(msg)  do { if (1 >= GS_LOG_MIN_LEVEL) \
::gs::Logger::Instance().Log(::gs::LogLevel::Info,  (msg)); } while (0)
#define LOG_WARN(msg)  do { if (2 >= GS_LOG_MIN_LEVEL) \
::gs::Logger::Instance().Log(::gs::LogLevel::Warn,  (msg)); } while (0)
#define LOG_ERROR(msg) do { if (3 >= GS_LOG_MIN_LEVEL) \
::gs::Logger::Instance().Log(::gs::LogLevel::Error, (msg)); } while (0)
