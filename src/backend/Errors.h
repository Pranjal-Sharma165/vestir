#pragma once

#include <stdexcept>

class VestirError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class NotFoundError : public VestirError {
  using VestirError::VestirError;
};

class InvalidInputError : public VestirError {
  using VestirError::VestirError;
};

class DatabaseError : public VestirError {
  using VestirError::VestirError;
};

class NetworkError : public VestirError {
  using VestirError::VestirError;
};