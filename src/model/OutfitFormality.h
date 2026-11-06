#ifndef OUTFITFORMALITY_H
#define OUTFITFORMALITY_H

#include <QQmlEngine>

/*!
 * \brief The OutfitFormality class defines the formality of an outfit, in the
 * context of how suitable is it to wear attending a `CalendarEvent`.
 */
class OutfitFormality : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON
public:
  /*!
   * \brief The Value enum defines the formality categories of an outfit.
   */
  enum class Value {
    Casual,
    SmartCasual,
    Formal,
    Sporty,
  };
  Q_ENUM(Value)

  OutfitFormality(QObject *parent = nullptr) : QObject(parent) {}

  /*!
   * \brief convert the `OutfitFormality::Value` enum to a user-facing string.
   * \param category the category to be converted.
   * \return A QString, intended to be displayed directly by the frontend.
   */
  Q_INVOKABLE QString displayName(OutfitFormality::Value category) const {
    switch (category) {
    case Value::Casual:
      return QString("Casual");
    case Value::SmartCasual:
      return QString("Smart Casual");
    case Value::Formal:
      return QString("Formal");
    case Value::Sporty:
      return QString("Sporty");
    }
  }

  /*!
   * \brief get a list of all category enum values.
   * \return A QVariantList containing all `OutfitFormality::Value` as entries.
   * Used by the frontend to display a list of the outfit formality values.
   */
  Q_INVOKABLE QVariantList getList() const {
    QVariantList list;
    QMetaEnum metaEnum = QMetaEnum::fromType<OutfitFormality::Value>();
    for (int i = 0; i < metaEnum.keyCount(); ++i) {
      list.append(QVariant::fromValue(
          static_cast<OutfitFormality::Value>(metaEnum.value(i))));
    }
    return list;
  }
};

#endif // OUTFITFORMALITY_H
