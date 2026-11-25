#ifndef CLOTHINGITEM_H
#define CLOTHINGITEM_H

#include "ClothingCategory.h"
#include "ClothingColour.h"
#include "OutfitFormality.h"
#include <QObject>
#include <QQmlEngine>
#include <filesystem>

class ClothingItem : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(QString qName READ qName CONSTANT)
  Q_PROPERTY(QUrl qImageUrl READ qImageUrl CONSTANT)
  Q_PROPERTY(ClothingCategory::Value category READ category CONSTANT)
  Q_PROPERTY(
      ClothingCategory::SubCategory subCategory READ subCategory CONSTANT)
  Q_PROPERTY(ClothingColour::Value colour READ colour CONSTANT)
  Q_PROPERTY(OutfitFormality::Value formality READ formality CONSTANT)

signals:

public:
  std::string m_name;
  std::filesystem::path m_imageUrl;
  ClothingCategory::Value m_category;
  ClothingCategory::SubCategory m_subCategory;
  ClothingColour::Value m_colour;
  OutfitFormality::Value m_formality;

  explicit ClothingItem(std::string name, ClothingCategory::Value category,
                        ClothingCategory::SubCategory subCategory,
                        ClothingColour::Value colour,
                        OutfitFormality::Value formality,
                        std::filesystem::path imageUrl,
                        QObject *parent = nullptr)
      : QObject(parent), m_name(name), m_category(category),
        m_subCategory(subCategory), m_colour(colour), m_formality(formality),
        m_imageUrl(imageUrl) {
    if (!ClothingCategory::subcategoryIsInCategory(subCategory, category)) {
      throw std::invalid_argument(
          std::format("Sub-category {} is not in category {}",
                      ClothingCategory::displayName(subCategory),
                      ClothingCategory::displayName(category)));
    }
  }

  /*!
   * \brief Copy the ClothingItem without copying the parent.
   * \param other the ClothingItem to copy.
   */
  ClothingItem(const ClothingItem &other)
      : ClothingItem(other.m_name, other.m_category, other.m_subCategory,
                     other.m_colour, other.m_formality, other.m_imageUrl,
                     nullptr) {}

  ClothingItem &operator=(const ClothingItem &other) {
    return *this = ClothingItem(other);
  }

  QString qName() const { return QString::fromStdString(m_name); }
  QUrl qImageUrl() const {
    return QUrl::fromLocalFile(QString::fromStdString(m_imageUrl.string()));
  }
  ClothingCategory::Value category() const { return m_category; }
  ClothingCategory::SubCategory subCategory() const { return m_subCategory; }
  ClothingColour::Value colour() const { return m_colour; }
  OutfitFormality::Value formality() const { return m_formality; }
  std::string name() const { return m_name; }
};

#endif // CLOTHINGITEM_H
