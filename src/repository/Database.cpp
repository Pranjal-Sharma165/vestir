#include "Database.h"
#include "PhotoStore.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace {

QString g_dataDir;

// A trigger body contains ';', so a CREATE TRIGGER statement only ends at a
// line that is "END;".
QStringList splitStatements(const QString &script) {
  QStringList result;
  QString current;
  bool inTrigger = false;
  for (QString line : script.split('\n')) {
    const int comment = line.indexOf("--");
    if (comment >= 0)
      line.truncate(comment);
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty())
      continue;
    if (current.isEmpty())
      inTrigger = trimmed.startsWith("CREATE TRIGGER", Qt::CaseInsensitive);
    current += line + '\n';
    const bool ends = inTrigger
                          ? trimmed.compare("END;", Qt::CaseInsensitive) == 0
                          : trimmed.endsWith(';');
    if (ends) {
      result << current;
      current.clear();
    }
  }
  return result;
}

bool runScript(QSqlDatabase &db, const QString &resource) {
  QFile file(resource);
  if (!file.open(QIODevice::ReadOnly)) {
    qWarning() << "Cannot read" << resource;
    return false;
  }
  for (const QString &sql :
       splitStatements(QString::fromUtf8(file.readAll()))) {
    QSqlQuery q(db);
    if (!q.exec(sql)) {
      qWarning() << "SQL failed:" << q.lastError().text() << "in"
                 << sql.left(80);
      return false;
    }
  }
  return true;
}

bool createDatabase(const QString &path) {
  const QString name = QStringLiteral("vestir-create");
  bool ok = false;
  {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", name);
    db.setDatabaseName(path);
    ok = db.open() && runScript(db, ":/db/schema.sql");
    db.close();
  }
  QSqlDatabase::removeDatabase(name);
  if (!ok)
    QFile::remove(path);
  return ok;
}

} // namespace

bool Database::initialize(const QString &dataDirOverride) {
  g_dataDir = dataDirOverride.isEmpty()
                  ? QStandardPaths::writableLocation(
                        QStandardPaths::AppDataLocation)
                  : dataDirOverride;

  if (!QDir().mkpath(g_dataDir) || !QDir().mkpath(wardrobeDir())) {
    qWarning() << "Cannot create data folder" << g_dataDir;
    return false;
  }

  const QString dbPath = QDir(g_dataDir).filePath("vestir.db");
  if (!QFileInfo::exists(dbPath) && !createDatabase(dbPath))
    return false;

  QSqlDatabase db = QSqlDatabase::contains(kConnectionName)
                        ? QSqlDatabase::database(kConnectionName)
                        : QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
  db.setDatabaseName(dbPath);
  if (!db.open()) {
    qWarning() << "Cannot open database:" << db.lastError().text();
    return false;
  }

  // SQLite ignores foreign keys unless this is enabled per connection.
  QSqlQuery(db).exec("PRAGMA foreign_keys = ON");

  PhotoStore::clearTempPhotos();
  return true;
}

QSqlDatabase Database::connection() {
  return QSqlDatabase::database(kConnectionName);
}

QString Database::dataDir() { return g_dataDir; }

QString Database::wardrobeDir() { return QDir(g_dataDir).filePath("wardrobe"); }

QString Database::absoluteImagePath(const QString &storedPath) {
  if (storedPath.isEmpty() || QDir::isAbsolutePath(storedPath))
    return storedPath;
  return QDir(g_dataDir).absoluteFilePath(storedPath);
}

QString Database::relativeImagePath(const QString &absolutePath) {
  return QDir(g_dataDir).relativeFilePath(absolutePath);
}

QString Database::setting(const QString &key) {
  QSqlQuery q(connection());
  q.prepare("SELECT value FROM app_settings WHERE key = :key");
  q.bindValue(":key", key);
  if (q.exec() && q.next())
    return q.value(0).toString();
  return {};
}

bool Database::setSetting(const QString &key, const QString &value) {
  QSqlQuery q(connection());
  q.prepare("INSERT INTO app_settings (key, value) VALUES (:k, :v) "
            "ON CONFLICT(key) DO UPDATE SET value = excluded.value");
  q.bindValue(":k", key);
  q.bindValue(":v", value);
  return q.exec();
}
