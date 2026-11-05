#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

/*!
 * \brief Opens the single-user SQLite database used by the backend.
 *
 * The file must already exist and contain the schema from
 * database/sqlite/schema.sql. On Android, copy the .db from app assets to a
 * writable location (e.g. QStandardPaths::AppDataLocation) before calling open.
 */
class Database {
public:
  /*! Opens the database at \a path. Returns false and logs on failure. */
  static bool open(const QString &path);

  /*! The shared connection. Only valid after a successful open(). */
  static QSqlDatabase connection();

private:
  static constexpr const char *kConnectionName = "vestir";
};

#endif // DATABASE_H
