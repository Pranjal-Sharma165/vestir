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

#include <filesystem>

#include "../../backend/PhotoStore.h"

/*!
 * \brief The CameraHelper class provides helper functions to the Camera page.
 */
class CameraHelper : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(QUrl photosDir READ photosDir CONSTANT)

public:
  explicit CameraHelper(PhotoStore &photoStore, QObject *parent = nullptr)
      : QObject(parent), m_photoStore(photoStore) {}

  QUrl photosDir() const {
    return QUrl::fromLocalFile(
        QString::fromStdString(m_photoStore.tempDir().string()));
  }

  Q_INVOKABLE bool ensureDir() const {
    return QDir().mkpath(photosDir().toLocalFile());
  }

  Q_INVOKABLE QUrl toUrl(const QString &path) const {
    return QUrl::fromLocalFile(path);
  }

  Q_INVOKABLE QString newPhotoPath() const {
    return QString::fromStdString(m_photoStore.newTempPath().string());
  }

  Q_INVOKABLE QUrl importPhoto(const QUrl &picked) const {
    qInfo() << "importPhoto: picked" << picked;
    const QString source = picked.isLocalFile() ? picked.toLocalFile() : picked.toString();
    const QString suffix = QFileInfo(source).suffix().toLower();
    const QString target =
        QString::fromStdString(m_photoStore
                                   .newTempPath(suffix.isEmpty()
                                                    ? std::string("jpg")
                                                    : suffix.toStdString())
                                   .string());
    if (!QFile::copy(source, target)) {
      qWarning() << "importPhoto: copy failed" << source << "->" << target;
      return {};
    }
    return QUrl::fromLocalFile(target);
  }

private:
  PhotoStore &m_photoStore;
};

#endif // CAMERAHELPER_H
