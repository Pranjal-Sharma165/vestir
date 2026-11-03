#ifndef CLOTHINGCATEGORY_H
#define CLOTHINGCATEGORY_H

#include <QQmlEngine>

/*!
 * \brief The ClothingCategory class defines the categories of clothes in
 * Vestir, and provides helper functions for the frontend to display them.
 */
class ClothingCategory : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  /*!
   * \brief The Value enum defines the categories of clothes in Vestir.
   */
  enum class Value {
    Top,
    Bottom,
    Outerwear,
    Shoe,
  };
  Q_ENUM(Value)

  ClothingCategory(QObject *parent = nullptr) : QObject(parent) {}

  /*!
   * \brief convert the category enum to a user-facing string.
   * \param category the category to be converted.
   * \return A QString, intended to be displayed directly by the frontend.
   */
  Q_INVOKABLE QString displayName(ClothingCategory::Value category) const {
    switch (category) {
    case Value::Top:
      return QString("Tops");
    case Value::Bottom:
      return QString("Bottoms");
    case Value::Outerwear:
      return QString("Outerwear");
    case Value::Shoe:
      return QString("Shoes");
    }
  }

  /*!
   * \brief get a list of all category enum values.
   * \return A QVariantList containing all `ClothingCategory::Value` as entries.
   * Used by the frontend to display a list of the categories.
   */
  Q_INVOKABLE QVariantList getList() const {
    QVariantList list;
    QMetaEnum metaEnum = QMetaEnum::fromType<ClothingCategory::Value>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      list.append(QVariant::fromValue(
          static_cast<ClothingCategory::Value>(metaEnum.value(i))));
    }
    return list;
  }
};

#endif // CLOTHINGCATEGORY_H
