#include "PhotoStore.h"
#include "Database.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QUuid>

namespace {

QString toLocalPath(const QString &path) {
  if (path.startsWith("file:", Qt::CaseInsensitive))
    return QUrl(path).toLocalFile();
  return path;
}

bool isInTempDir(const QString &path) {
  const QString dir = QDir(PhotoStore::tempDir()).canonicalPath();
  const QString file = QFileInfo(path).canonicalFilePath();
  return !dir.isEmpty() && !file.isEmpty() &&
         file.startsWith(dir + QLatin1Char('/'));
}

} // namespace

QString PhotoStore::tempDir() {
  return QDir(Database::dataDir()).filePath("photos");
}

QString PhotoStore::addPhoto(const QString &tempPath) {
  const QString source = toLocalPath(tempPath);
  if (!QFileInfo::exists(source)) {
    qWarning() << "Photo not found:" << source;
    return {};
  }

  QString suffix = QFileInfo(source).suffix().toLower();
  if (suffix.isEmpty())
    suffix = "jpg";
  const QString name = "IMG_" +
                       QUuid::createUuid().toString(QUuid::WithoutBraces).left(12) +
                       "." + suffix;
  const QString target = QDir(Database::wardrobeDir()).filePath(name);

  if (!QFile::copy(source, target)) {
    qWarning() << "Cannot copy photo to" << target;
    return {};
  }
  if (isInTempDir(source))
    QFile::remove(source);
  return Database::relativeImagePath(target);
}

void PhotoStore::discardPhoto(const QString &tempPath) {
  const QString source = toLocalPath(tempPath);
  if (isInTempDir(source))
    QFile::remove(source);
}

void PhotoStore::clearTempPhotos() {
  const QDir dir(tempDir());
  for (const QString &name : dir.entryList(QDir::Files))
    QFile::remove(dir.filePath(name));
}
