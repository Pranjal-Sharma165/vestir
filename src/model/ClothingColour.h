#ifndef CLOTHINGCOLOUR_H
#define CLOTHINGCOLOUR_H

#include <QQmlEngine>

#define COLOUR_LIST(X)                                                         \
  X(Black)                                                                     \
  X(White)                                                                     \
  X(Charcoal)                                                                  \
  X(Brown)                                                                     \
  X(Beige)                                                                     \
  X(Khaki)                                                                     \
  X(Cream)                                                                     \
  X(NavyBlue)                                                                  \
  X(Blue)                                                                      \
  X(LightBlue)                                                                 \
  X(Green)                                                                     \
  X(OliveGreen)                                                                \
  X(Red)                                                                       \
  X(Pink)                                                                      \
  X(Purple)                                                                    \
  X(Yellow)                                                                    \
  X(Orange)                                                                    \
  X(Burgundy)

/*!
 * \brief The ClothingColour class defines the different primary colours of
 * clothes in Vestir, and provides helper functions for the frontend to display
 * them.
 */
class ClothingColour : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
#define DEFINE_ENUM(name) name,
  /*!
   * \brief The Value enum defines the primary colours of clothes in Vestir.
   */
  enum class Value { COLOUR_LIST(DEFINE_ENUM) };
#undef DEFINE_ENUM

  Q_ENUM(Value)

  ClothingColour(QObject *parent = nullptr) : QObject(parent) {}

  /*!
   * \brief convert the colour enum to a user-facing string.
   * \param category the category to be converted.
   * \return A QString, intended to be displayed directly by the frontend.
   */
  Q_INVOKABLE QString displayName(ClothingColour::Value colour) const {
    return QString::fromUtf8(m_colourStrings[static_cast<size_t>(colour)]);
  }

  /*!
   * \brief get a list of all colour enum values.
   * \return A QVariantList containing all `ClothingColour::Value` as entries.
   * Used by the frontend to display a list of clothing colours.
   */
  Q_INVOKABLE QVariantList getList() const {
    QVariantList list;
    QMetaEnum metaEnum = QMetaEnum::fromType<ClothingColour::Value>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      list.append(QVariant::fromValue(
          static_cast<ClothingColour::Value>(metaEnum.value(i))));
    }
    return list;
  }

private:
#define DEFINE_STRING(name) std::string_view(#name),
  static inline constexpr auto m_colourStrings =
      std::array{COLOUR_LIST(DEFINE_STRING)};
#undef DEFINE_STRING
};

#undef COLOUR_LIST

#endif // CLOTHINGCOLOUR_H
