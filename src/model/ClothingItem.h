#ifndef CLOTHINGITEM_H
#define CLOTHINGITEM_H

#include "ClothingCategory.h"
#include <QObject>
#include <QQmlEngine>

class ClothingItem : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  // CONSTANT for now; needs to change if the user is allowed to change these
  // values (likely).
  Q_PROPERTY(QString nameQString READ nameQString CONSTANT)
  Q_PROPERTY(ClothingCategory::Value category READ category CONSTANT)

signals:

public:
  explicit ClothingItem(std::string name, ClothingCategory::Value category,
                        QObject *parent)
      : QObject(parent), m_name(name), m_category(category) {}

  QString nameQString() const { return QString::fromStdString(m_name); }
  ClothingCategory::Value category() const { return m_category; }
  std::string name() const { return m_name; }

private:
  std::string m_name;
  ClothingCategory::Value m_category;
};

#endif // CLOTHINGITEM_H
