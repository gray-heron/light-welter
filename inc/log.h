#pragma once

#include <sstream>
#include <string>

#include "spdlog/spdlog.h"

class LogStream;
class Log;

class LoggingSingleton
{
  private:
    LoggingSingleton();
    std::vector<spdlog::sink_ptr> sinks_;
    std::vector<std::shared_ptr<spdlog::logger>> handles_;

  public:
    LoggingSingleton(LoggingSingleton const &) = delete;
    void operator=(LoggingSingleton const &) = delete;

    static LoggingSingleton &inst()
    {
        static LoggingSingleton instance;
        return instance;
    }

    void SetConsoleVerbosity(bool verbose);
    void AddLogFile(std::string name);

    std::shared_ptr<spdlog::logger> RegisterModule(std::string name);
};

class LogStream
{
    friend class Log;

    std::shared_ptr<spdlog::logger> handle_;
    std::stringstream stream_;
    spdlog::level::level_enum level_;

    LogStream(std::shared_ptr<spdlog::logger>, spdlog::level::level_enum level);
    LogStream(const LogStream &);

  public:
    template <typename T> std::ostream &operator<<(const T &msg)
    {
        stream_ << msg;
        return stream_;
    }

    ~LogStream();
};

class Log
{
    std::string module_;

    std::weak_ptr<spdlog::logger> handle_;
    std::shared_ptr<spdlog::logger> GetHandle();

  public:
    Log(std::string module_name);
    LogStream Debug();
    LogStream Info();
    LogStream Warning();
    LogStream Error();
};
