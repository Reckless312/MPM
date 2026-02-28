#ifndef MPM_METHOD_PROGRAMEXCEPTION_H
#define MPM_METHOD_PROGRAMEXCEPTION_H
#include <exception>
#include <string>

#include "Error.h"

class MPMException: public std::exception
{
public:
    explicit MPMException(const char* message, Error errorType);
    [[nodiscard]] const char* what() const noexcept override;

    [[nodiscard]] Error GetErrorType() const;
private:
    std::string message;

    Error errorType;
};

#endif