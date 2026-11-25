#ifndef OUTFITFORMALITY_H
#define OUTFITFORMALITY_H

#include <QQmlEngine>

#define FORMALITY_LIST(X)                                                      \
  X(Casual, Casual)                                                            \
  X(SmartCasual, Smart Casual)                                                 \
  X(Formal, Formal)                                                            \
  X(Sporty, Sporty)

/*!
 * \brief The OutfitFormality class defines the formality of an outfit, in the
 * context of how suitable is it to wear attending a `CalendarEvent`.
 */
class OutfitFormality : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
#define DEFINE_FORMALITIES_ENUM(symbol, string) symbol,
  /*!
   * \brief The Value enum defines the formality categories of an outfit.
   */
  enum class Value { FORMALITY_LIST(DEFINE_FORMALITIES_ENUM) };
#undef DEFINE_FORMALITIES_ENUM
  Q_ENUM(Value)

  OutfitFormality(QObject *parent = nullptr) : QObject(parent) {}

  /*!
   * \brief convert the `OutfitFormality::Value` enum to a user-facing string.
   * \param formality the formality to be converted.
   * \return A QString, intended to be displayed directly by the frontend.
   */
  Q_INVOKABLE QString qDisplayName(OutfitFormality::Value formality) const {
    return QString::fromUtf8(
        m_formalityStrings[static_cast<size_t>(formality)]);
  }

  static std::string_view displayName(OutfitFormality::Value formality) {
    return m_formalityStrings[static_cast<size_t>(formality)];
  }

  /*!
   * \brief get a list of all category enum values.
   * \return A QVariantList containing all `OutfitFormality::Value` as entries.
   * Used by the frontend to display a list of the outfit formality values.
   */
  Q_INVOKABLE QVariantList qGetList() const {
    QVariantList list;
    QMetaEnum metaEnum = QMetaEnum::fromType<OutfitFormality::Value>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      list.append(QVariant::fromValue(
          static_cast<OutfitFormality::Value>(metaEnum.value(i))));
    }
    return list;
  }

private:
#define DEFINE_FORMALITY_STRING(symbol, string) std::string_view(#string),
  static inline constexpr auto m_formalityStrings =
      std::array{FORMALITY_LIST(DEFINE_FORMALITY_STRING)};
#undef DEFINE_FORMALITY_STRING
};

#endif // OUTFITFORMALITY_H
