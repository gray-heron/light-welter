
#include <iostream>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/spdlog.h>

#include "config.h"
#include "log.h"

LoggingSingleton::LoggingSingleton()
{
    try
    {
        auto console_sink = std::make_shared<spdlog::sinks::ansicolor_stdout_sink_mt>();

        console_sink->set_level(spdlog::level::info);
        // console_sink->set_pattern("[multi_sink_example] [%^%l%$] %v");
        sinks_.push_back(console_sink);
    }
    catch (const spdlog::spdlog_ex &ex)
    {
        std::cout << "Log initialization failed: " << ex.what() << std::endl;
    }
}

void LoggingSingleton::SetConsoleVerbosity(bool verbose)
{
    sinks_[0]->set_level(verbose ? spdlog::level::debug : spdlog::level::info);
}

void LoggingSingleton::AddLogFile(std::string name)
{
    auto file_sink = std::make_shared<spdlog::sinks::simple_file_sink_mt>(name, true);

    file_sink->set_level(spdlog::level::trace);

    sinks_.push_back(file_sink);
    handles_.clear();
}

std::shared_ptr<spdlog::logger> LoggingSingleton::RegisterModule(std::string name)
{
    std::shared_ptr<spdlog::logger> ret =
        std::make_shared<spdlog::logger>(name, std::begin(sinks_), std::end(sinks_));

    handles_.push_back(ret);
    return ret;
}

LogStream::LogStream(std::shared_ptr<spdlog::logger> handle,
                     spdlog::level::level_enum level)
    : handle_(handle), level_(level)
{
}

LogStream::LogStream(const LogStream &oth) : handle_(oth.handle_), level_(oth.level_) {}

LogStream::~LogStream()
{
    stream_.seekg(0, stream_.end);
    if (stream_.tellg() > 0)
        handle_->log(level_, stream_.str());
}

Log::Log(std::string module_name) : module_(module_name) {}

std::shared_ptr<spdlog::logger> Log::GetHandle()
{
    auto ret = handle_.lock();
    if (!ret)
    {
        ret = LoggingSingleton::inst().RegisterModule(module_);
    }

    return ret;
}

LogStream Log::Debug() { return LogStream(GetHandle(), spdlog::level::debug); }

LogStream Log::Info() { return LogStream(GetHandle(), spdlog::level::info); }

LogStream Log::Warning() { return LogStream(GetHandle(), spdlog::level::warn); }

LogStream Log::Error() { return LogStream(GetHandle(), spdlog::level::err); }