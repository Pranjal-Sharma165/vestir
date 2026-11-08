#ifndef ERRORS_H
#define ERRORS_H

#include <QString>
#include <stdexcept>

/*!
 * Errors thrown by the backend. Catch VestirError to handle all of them, or a
 * subclass to react differently. Page-model slots must catch them: an
 * exception that escapes into Qt's event loop crashes the app.
 */
class VestirError : public std::runtime_error {
public:
  explicit VestirError(const QString &message)
      : std::runtime_error(message.toStdString()), m_message(message) {}
  QString message() const { return m_message; }

private:
  QString m_message;
};

/*! The requested item does not exist. */
class NotFoundError : public VestirError {
  using VestirError::VestirError;
};

/*! The caller passed a value that is not allowed (bad link, unknown label). */
class InvalidInputError : public VestirError {
  using VestirError::VestirError;
};

/*! The local database could not be read or written. */
class DatabaseError : public VestirError {
  using VestirError::VestirError;
};

/*! A network request failed (no internet, timeout, server error). */
class NetworkError : public VestirError {
  using VestirError::VestirError;
};

#endif // ERRORS_H
