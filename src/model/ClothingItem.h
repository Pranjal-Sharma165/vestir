#ifndef CLOTHINGITEM_H
#define CLOTHINGITEM_H

#include "ClothingCategory.h"
#include <QObject>
#include <QQmlEngine>

class ClothingItem : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(QString name READ name CONSTANT)
  Q_PROPERTY(QString category READ category CONSTANT)

signals:

public:
  explicit ClothingItem(std::string name, ClothingCategory category,
                        QObject *parent)
      : QObject(parent), m_name(name), m_category(category) {}

  QString name() const { return QString::fromStdString(m_name); }
  QString category() const {
    switch (m_category) {
    case ClothingCategory::Top:
      return QString("Tops");
    case ClothingCategory::Bottom:
      return QString("Bottoms");
    case ClothingCategory::Outerwear:
      return QString("Outerwear");
    case ClothingCategory::Shoe:
      return QString("Shoes");
    }
  }

private:
  std::string m_name;
  ClothingCategory m_category;
};

#endif // CLOTHINGITEM_H
