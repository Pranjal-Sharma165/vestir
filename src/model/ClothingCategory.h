#ifndef CLOTHINGCATEGORY_H
#define CLOTHINGCATEGORY_H

#include <QQmlEngine>

#define SUBCATEGORY_LIST(X)                                                    \
  X(TShirt, Top)                                                               \
  X(Blouse, Top)                                                               \
  X(Sweater, Top)                                                              \
  X(Hoodie, Top)                                                               \
  X(Shirt, Top)                                                                \
  X(TankTop, Top)                                                              \
  X(Jeans, Bottom)                                                             \
  X(Trousers, Bottom)                                                          \
  X(Shorts, Bottom)                                                            \
  X(Skirt, Bottom)                                                             \
  X(Leggings, Bottom)                                                          \
  X(Dress, Dress)                                                              \
  X(Jumpsuit, Dress)                                                           \
  X(Romper, Dress)                                                             \
  X(Jacket, Outerwear)                                                         \
  X(Coat, Outerwear)                                                           \
  X(Blazer, Outerwear)                                                         \
  X(Parka, Outerwear)                                                          \
  X(Cardigan, Outerwear)                                                       \
  X(Vest, Outerwear)

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
    Dress,
  };
  Q_ENUM(Value)

#define DEFINE_SUBCATEGORIES_ENUM(child, parent) child,
  /*!
   * \brief The SubCategory enum defines all the subcategories for each
   * category.
   */
  enum class SubCategory { SUBCATEGORY_LIST(DEFINE_SUBCATEGORIES_ENUM) };
#undef DEFINE_SUBCATEGORIES_ENUM

  Q_ENUM(SubCategory)

  ClothingCategory(QObject *parent = nullptr) : QObject(parent) {}

  /*!
   * \brief convert the category enum to a user-facing string.
   * \param category the category to be converted.
   * \return A QString, intended to be displayed directly by the frontend.
   */
  Q_INVOKABLE QString
  qCategoryDisplayName(ClothingCategory::Value category) const {
    return QString::fromUtf8(displayName(category));
  }

  /*!
   * \brief Convert the subcategory enum to a user-facing string.
   * \param subcategory the subcategory to be converted.
   * \return A QString, intended to be displayed directly by the frontend.
   */
  Q_INVOKABLE QString
  qSubCategoryDisplayName(ClothingCategory::SubCategory subCategory) const {
    return QString::fromUtf8(displayName(subCategory));
  }

  /*!
   * \brief Get a list of all category enum values.
   * \return A QVariantList containing all `ClothingCategory::Value` values as
   * entries. Used by the frontend to display a list of the categories.
   */
  Q_INVOKABLE QVariantList qGetList() const {
    QVariantList list;
    QMetaEnum metaEnum = QMetaEnum::fromType<Value>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      list.append(QVariant::fromValue(static_cast<Value>(metaEnum.value(i))));
    }
    return list;
  }

  /*!
   * \brief Get a list of all subcategory enum values for a given category.
   * \return A QVariantList containing all `ClothingCategory::SubCategory`
   * values as entries. Used by the frontend to display a list of subcategories
   * for a particular category.
   */
  Q_INVOKABLE QVariantList qGetList(Value category) const {
    QVariantList list;
    QMetaEnum metaEnum = QMetaEnum::fromType<SubCategory>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      if (m_subcategoryMap.at(static_cast<SubCategory>(i)) == category) {
        list.append(QVariant::fromValue(static_cast<Value>(metaEnum.value(i))));
      }
    }
    return list;
  }

  static inline bool subcategoryIsInCategory(SubCategory subCategory,
                                             Value category) {
    return m_subcategoryMap.at(subCategory) == category;
  }

  static inline std::string_view displayName(Value category) {
    switch (category) {
    case Value::Top:
      return "Tops";
    case Value::Bottom:
      return "Bottoms";
    case Value::Outerwear:
      return "Outerwear";
    case Value::Dress:
      return "Dresses";
    }
  }

  static inline std::string_view displayName(SubCategory subCategory) {
    return m_subCategoryStrings[static_cast<size_t>(subCategory)];
  }

private:
#define DEFINE_SUBCATEGORIES_MAPPING(child, parent)                            \
  {SubCategory::child, Value::parent},
  static inline const std::map<SubCategory, Value> m_subcategoryMap = {
      SUBCATEGORY_LIST(DEFINE_SUBCATEGORIES_MAPPING)};
#undef DEFINE_SUBCATEGORIES_MAPPING

#define DEFINE_SUBCATEGORY_STRING(child, parent) std::string_view(#child),
  static inline constexpr auto m_subCategoryStrings =
      std::array{SUBCATEGORY_LIST(DEFINE_SUBCATEGORY_STRING)};
#undef DEFINE_SUBCATEGORY_STRING
};

#undef SUBCATEGORY_LIST

#endif // CLOTHINGCATEGORY_H
