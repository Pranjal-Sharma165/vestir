#ifndef CAMERAHELPER_H
#define CAMERAHELPER_H
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QObject>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

#include "../../repository/PhotoStore.h"

/*!
 * \brief The CameraHelper class provides helper functions to the Camera page.
 */
class CameraHelper : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

  Q_PROPERTY(QUrl photosDir READ photosDir CONSTANT)

public:
  QUrl photosDir() const {
    return QUrl::fromLocalFile(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        QStringLiteral("/photos"));
  }

  Q_INVOKABLE bool ensureDir() const {
    return QDir().mkpath(photosDir().toLocalFile());
  }

  Q_INVOKABLE QUrl toUrl(const QString &path) const {
    return QUrl::fromLocalFile(path);
  }

  Q_INVOKABLE QString newPhotoPath() const {
    return PhotoStore::newTempPath();
  }

  /*!
   * TODO Android: the gallery picker returns a content:// URL there, and
   * toLocalFile()/QFile::copy() may not work with it. Untested on a phone.
   * 
   * Copies a photo picked from the gallery into the temporary photos folder,
   * so it is handled like a camera shot. Returns the copy's URL, or an empty
   * URL if the copy failed.
   */
  Q_INVOKABLE QUrl importPhoto(const QUrl &picked) const {
    const QString source = picked.toLocalFile();
    const QString suffix = QFileInfo(source).suffix().toLower();
    const QString target =
        PhotoStore::newTempPath(suffix.isEmpty() ? QString("jpg") : suffix);
    if (!QFile::copy(source, target))
      return {};
    return QUrl::fromLocalFile(target);
  }
};

#endif // CAMERAHELPER_H
