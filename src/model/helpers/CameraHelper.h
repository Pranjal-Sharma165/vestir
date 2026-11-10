#ifndef CAMERAHELPER_H
#define CAMERAHELPER_H
#include <QDebug>
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
   * Copies a photo picked from the gallery into the temporary photos folder,
   * so it is handled like a camera shot. Returns the copy's URL, or an empty
   * URL if the copy failed.
   */
  Q_INVOKABLE QUrl importPhoto(const QUrl &picked) const {
    // Android's gallery returns content:// URLs, which QFile opens by URL string.
    qInfo() << "importPhoto: picked" << picked;
    const QString source = picked.isLocalFile() ? picked.toLocalFile() : picked.toString();
    const QString suffix = QFileInfo(source).suffix().toLower();
    const QString target =
        PhotoStore::newTempPath(suffix.isEmpty() ? QString("jpg") : suffix);
    if (!QFile::copy(source, target)) {
      qWarning() << "importPhoto: copy failed" << source << "->" << target;
      return {};
    }
    return QUrl::fromLocalFile(target);
  }
};

#endif // CAMERAHELPER_H
