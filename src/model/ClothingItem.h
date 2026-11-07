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
  Q_PROPERTY(
      ClothingCategory::SubCategory subCategory READ subCategory CONSTANT)
  Q_PROPERTY(ClothingColour::Value colour READ colour CONSTANT)

signals:

public:
  explicit ClothingItem(std::string name, ClothingCategory::Value category,
                        ClothingCategory::SubCategory subCategory,
                        ClothingColour::Value colour, QObject *parent)
      : QObject(parent), m_name(name), m_category(category),
        m_subCategory(subCategory), m_colour(colour) {
    if (!ClothingCategory::subcategoryIsInCategory(subCategory, category)) {
      throw std::invalid_argument(
          std::format("Sub-category {} is not in category {}",
                      ClothingCategory::displayName(subCategory),
                      ClothingCategory::displayName(category)));
    }
  }

  QString qName() const { return QString::fromStdString(m_name); }
  ClothingCategory::Value category() const { return m_category; }
  ClothingCategory::SubCategory subCategory() const { return m_subCategory; }
  ClothingColour::Value colour() const { return m_colour; }
  std::string name() const { return m_name; }

private:
  std::string m_name;
  ClothingCategory::Value m_category;
  ClothingCategory::SubCategory m_subCategory;
  ClothingColour::Value m_colour;
};

#endif // CLOTHINGITEM_H
