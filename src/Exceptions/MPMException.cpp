#include "MPMException.h"

MPMException::MPMException(const char *message, const Error errorType)
{
    this->message = message;
    this->errorType = errorType;
}

const char * MPMException::what() const noexcept
{
    return this->message.c_str();
}

Error MPMException::GetErrorType() const
{
    return this->errorType;
}
