#ifndef CLOTHINGITEM_H
#define CLOTHINGITEM_H

#include "ClothingCategory.h"
#include "ClothingColour.h"
#include <QObject>
#include <QQmlEngine>

class ClothingItem : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(QString qName READ qName CONSTANT)
  Q_PROPERTY(ClothingCategory::Value category READ category CONSTANT)
  Q_PROPERTY(ClothingColour::Value colour READ colour CONSTANT)

signals:

public:
  explicit ClothingItem(std::string name, ClothingCategory::Value category,
                        ClothingColour::Value colour, QObject *parent)
      : QObject(parent), m_name(name), m_category(category), m_colour(colour) {}

  QString qName() const { return QString::fromStdString(m_name); }
  ClothingCategory::Value category() const { return m_category; }
  ClothingColour::Value colour() const { return m_colour; }
  std::string name() const { return m_name; }

private:
  std::string m_name;
  ClothingCategory::Value m_category;
  ClothingColour::Value m_colour;
};

#endif // CLOTHINGITEM_H
