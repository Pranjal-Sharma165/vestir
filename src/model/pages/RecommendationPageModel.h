#ifndef RECOMMENDATIONPAGEMODEL_H
#define RECOMMENDATIONPAGEMODEL_H

#include "ClothingCategory.h"
#include "OutfitFormality.h"
#include "OutfitRecommendation.h"
#include <QQmlEngine>

/*!
 * \brief The RecommendationPageModel class is the data source for the outfit
 * recommendations page.
 */
class RecommendationPageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QList<OutfitRecommendation *> outfits READ outfits NOTIFY
                 outfitsListChanged)

signals:
  /*!
   * Signal that the contents of the outfits list have changed.
   */
  void outfitsListChanged();

public:
  explicit RecommendationPageModel(QObject *parent = nullptr)
      : QObject(parent) {
    // TODO replace mock data with real call to database
    QList<ClothingItem *> outfit1Items({
        new ClothingItem("Outfit 1 Shirt", ClothingCategory::Value::Top,
                         ClothingCategory::SubCategory::Shirt,
                         ClothingColour::Value::Red,
                         OutfitFormality::Value::Casual, this),
        new ClothingItem("Outfit 1 Pants", ClothingCategory::Value::Bottom,
                         ClothingCategory::SubCategory::Jeans,
                         ClothingColour::Value::Blue,
                         OutfitFormality::Value::SmartCasual, this),
        new ClothingItem("Outfit 1 Dress", ClothingCategory::Value::Dress,
                         ClothingCategory::SubCategory::Dress,
                         ClothingColour::Value::Green,
                         OutfitFormality::Value::Formal, this),
        new ClothingItem("Outfit 1 Coat", ClothingCategory::Value::Outerwear,
                         ClothingCategory::SubCategory::Coat,
                         ClothingColour::Value::Black,
                         OutfitFormality::Value::Sporty, this),
    });
    m_outfits.append(new OutfitRecommendation(
        outfit1Items, "Your morning outfit", OutfitFormality::Value::Casual,
        nullptr, nullptr, this));

    QList<ClothingItem *> outfit2Items({
        new ClothingItem("Outfit 2 Shirt", ClothingCategory::Value::Top,
                         ClothingCategory::SubCategory::Shirt,
                         ClothingColour::Value::Red,
                         OutfitFormality::Value::Casual, this),
        new ClothingItem("Outfit 2 Pants", ClothingCategory::Value::Bottom,
                         ClothingCategory::SubCategory::Jeans,
                         ClothingColour::Value::Blue,
                         OutfitFormality::Value::SmartCasual, this),
        new ClothingItem("Outfit 2 Dress", ClothingCategory::Value::Dress,
                         ClothingCategory::SubCategory::Dress,
                         ClothingColour::Value::Green,
                         OutfitFormality::Value::Formal, this),
    });
    m_outfits.append(new OutfitRecommendation(
        outfit2Items, "Your evening outfit", OutfitFormality::Value::Formal,
        nullptr, nullptr, this));
  }

  QList<OutfitRecommendation *> outfits() const { return m_outfits; }

private:
  QList<OutfitRecommendation *> m_outfits;
};

#endif // RECOMMENDATIONPAGEMODEL_H
