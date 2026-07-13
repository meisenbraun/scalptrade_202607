#pragma once
#include <exception>

enum exit_statusEnum
{
    exit_status_success = 0,
    exit_status_fail = 1,
    exit_status_args = 3
};

class ProgramException : public std::exception
{
public:
    explicit ProgramException(exit_statusEnum code) :
    std::exception(),
    code_(code)
    {}

    exit_statusEnum code() const {return code_;}

private:
    exit_statusEnum code_;
};