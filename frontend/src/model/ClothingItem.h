#ifndef CLOTHINGITEM_H
#define CLOTHINGITEM_H

#include "ClothingCategory.h"
#include "../backend/ImagePath.h"
#include <QObject>
#include <QQmlEngine>
#include <QUrl>

class ClothingItem : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  // CONSTANT for now; needs to change if the user is allowed to change these
  // values (likely).
  Q_PROPERTY(QString nameQString READ nameQString CONSTANT)
  Q_PROPERTY(ClothingCategory::Value category READ category CONSTANT)
  Q_PROPERTY(int id READ id CONSTANT)
  Q_PROPERTY(QUrl imageUrl READ imageUrl CONSTANT)

signals:

public:
  // id is the database row id (0 if not stored); imagePath is a local file.
  explicit ClothingItem(std::string name, ClothingCategory::Value category,
                        QObject *parent, int id = 0,
                        const QString &imagePath = QString())
      : QObject(parent), m_name(name), m_category(category), m_id(id),
        m_imagePath(imagePath) {}

  int id() const { return m_id; }

  // Empty URL if the item has no image.
  QUrl imageUrl() const {
    return imageUrlFromPath(m_imagePath);
  }

  QString nameQString() const { return QString::fromStdString(m_name); }
  ClothingCategory::Value category() const { return m_category; }
  std::string name() const { return m_name; }

private:
  std::string m_name;
  ClothingCategory::Value m_category;
  int m_id;
  QString m_imagePath;
};

#endif // CLOTHINGITEM_H
