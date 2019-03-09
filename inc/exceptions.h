#pragma once
#include <csignal>
#include <cstring>
#include <exception>
#include <string>

#include "log.h"

#define __3RD_ARGUMENT(a1, a2, a3, ...) a3

#define __ASSERT_SWITCH(...) __3RD_ARGUMENT(__VA_ARGS__, __ASSERT2, __ASSERT1)

#define __ASSERT2(x, y)                                                                  \
    if (!(x))                                                                            \
        throw AssertionFailedException(__FILE__, __LINE__, y);

#define __ASSERT1(x)                                                                     \
    if (!(x))                                                                            \
        throw AssertionFailedException(__FILE__, __LINE__);

#define ASSERT(...) __ASSERT_SWITCH(__VA_ARGS__)(__VA_ARGS__)

class Exception : public std::exception
{
  protected:
  public:
    std::string msg_;

    const char *what() const throw() { return msg_.c_str(); };

    virtual ~Exception() throw(){};
    Exception(std::string what) : msg_(what) { Log("Exception").Error() << msg_; }
};

class AssertionFailedException : Exception
{
  private:
  public:
    AssertionFailedException(const char *file, int line, std::string msg = "")
        : Exception("")
    {
        msg_ = (std::string) "Assertion at " + std::string(file) + ":" +
               std::to_string(line) + " has failed";
        if (msg == "")
            msg_ += ".";
        else
            msg_ += " (" + msg + ").";

        Log("Exception").Error() << msg_;
    }
};
