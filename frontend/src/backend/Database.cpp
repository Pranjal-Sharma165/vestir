#include "Database.h"

#include <QDebug>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>

bool Database::open(const QString &path) {
  if (!QFileInfo::exists(path)) {
    qWarning() << "Database file not found:" << path;
    return false;
  }

  QSqlDatabase db = QSqlDatabase::contains(kConnectionName)
                        ? QSqlDatabase::database(kConnectionName)
                        : QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
  db.setDatabaseName(path);

  if (!db.open()) {
    qWarning() << "Cannot open database:" << db.lastError().text();
    return false;
  }

  // SQLite ignores foreign keys unless this is enabled per connection.
  QSqlQuery(db).exec("PRAGMA foreign_keys = ON");
  return true;
}

QSqlDatabase Database::connection() {
  return QSqlDatabase::database(kConnectionName);
}
