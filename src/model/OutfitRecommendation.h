#ifndef OUTFITRECOMMENDATION_H
#define OUTFITRECOMMENDATION_H

#include "CalendarEvent.h"
#include "ClothingItem.h"
#include "OutfitFormality.h"
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class OutfitRecommendation : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(int qPieceCount READ qPieceCount CONSTANT)
  Q_PROPERTY(QList<ClothingItem *> qPieceList READ qPieceList CONSTANT)
  Q_PROPERTY(QString qTitle READ qTitle CONSTANT)
  Q_PROPERTY(CalendarEvent *qEvent READ qEvent CONSTANT)
  Q_PROPERTY(QVariantMap qWeather READ qWeather CONSTANT)
  Q_PROPERTY(qint64 qRecommendationId READ qRecommendationId CONSTANT)

public:
  QList<ClothingItem *> m_clothingItems;
  std::string m_title;
  OutfitFormality::Value m_formality;
  CalendarEvent *m_event;
  QVariantMap m_weather;
  qint64 m_recommendationId;

  explicit OutfitRecommendation(QList<ClothingItem *> items, std::string title,
                                OutfitFormality::Value formality,
                                CalendarEvent *event = nullptr,
                                QVariantMap weather = {},
                                QObject *parent = nullptr,
                                qint64 recommendationId = -1)
      : m_clothingItems(items), m_title(title), m_formality(formality),
        m_event(event), m_weather(weather),
        m_recommendationId(recommendationId), QObject(parent) {}

  /*!
   * \brief Copy the OutfitRecommendation without copying the parent.
   * \param other the OutfitRecommendation to copy.
   */
  OutfitRecommendation(const OutfitRecommendation &other)
      : OutfitRecommendation(other.m_clothingItems, other.m_title,
                             other.m_formality, other.m_event, other.m_weather,
                             nullptr, other.m_recommendationId) {}

  OutfitRecommendation &operator=(const OutfitRecommendation &other) {
    return *this = OutfitRecommendation(other);
  }

  int const qPieceCount() const { return m_clothingItems.size(); }

  QString qTitle() const { return QString::fromStdString(m_title); }
  CalendarEvent *qEvent() const { return m_event; }

  QList<ClothingItem *> qPieceList() const { return m_clothingItems; }

  QVariantMap qWeather() const { return m_weather; }

  // Database ID of this recommendation, used to save choose/reject and to
  // find it in the history. -1 if it was not saved.
  qint64 qRecommendationId() const { return m_recommendationId; }
};

#endif // OUTFITRECOMMENDATION_H
