#ifndef CAMERAHELPER_H
#define CAMERAHELPER_H
#include <QDir>
#include <QObject>
#include <QQmlEngine>
#include <QStandardPaths>
#include <QUrl>

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
    ensureDir();
    return photosDir().toLocalFile() + QStringLiteral("/photo_") +
           QString::number(QDateTime::currentMSecsSinceEpoch()) +
           QStringLiteral(".jpg");
  }
};

#endif // CAMERAHELPER_H
