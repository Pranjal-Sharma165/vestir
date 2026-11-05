#ifndef IMAGEPATH_H
#define IMAGEPATH_H

#include <QDir>
#include <QString>
#include <QUrl>

/*!
 * \brief Turns a stored image_path into a file URL.
 *
 * Paths are stored relative to the project root (e.g. "wardrobe/IMG_1.jpg") so
 * the database works on every machine. Absolute paths are used as they are.
 * VESTIR_ROOT_DIR is the project root, set by CMake.
 */
inline QUrl imageUrlFromPath(const QString &path) {
  if (path.isEmpty())
    return {};
  if (QDir::isAbsolutePath(path))
    return QUrl::fromLocalFile(path);
  return QUrl::fromLocalFile(QDir(QStringLiteral(VESTIR_ROOT_DIR))
                                 .absoluteFilePath(path));
}

#endif // IMAGEPATH_H
