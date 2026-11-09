#ifndef OUTFITRECOMMENDATION_H
#define OUTFITRECOMMENDATION_H

#include "CalendarEvent.h"
#include "ClothingItem.h"
#include "OutfitFormality.h"
#include "WeatherSnapshot.h"
#include <QObject>
#include <QtQml/qqmlregistration.h>

class OutfitRecommendation : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(int qPieceCount READ qPieceCount CONSTANT)
  Q_PROPERTY(QList<ClothingItem *> qPieceList READ qPieceList CONSTANT)
  Q_PROPERTY(QString qTitle READ qTitle CONSTANT)
  Q_PROPERTY(CalendarEvent *qEvent READ qEvent CONSTANT)
  Q_PROPERTY(WeatherSnapshot *qWeather READ qWeather CONSTANT)
  Q_PROPERTY(qint64 qRecommendationId READ qRecommendationId CONSTANT)

signals:

public:
  explicit OutfitRecommendation(QList<ClothingItem *> items, std::string title,
                                OutfitFormality::Value formality,
                                CalendarEvent *event = nullptr,
                                WeatherSnapshot *weather = nullptr,
                                QObject *parent = nullptr,
                                qint64 recommendationId = -1)
      : m_clothingItems(items), m_title(title), m_event(event),
        m_weather(weather), m_recommendationId(recommendationId),
        QObject(parent) {}

  int const qPieceCount() const { return m_clothingItems.size(); }

  QString qTitle() const { return QString::fromStdString(m_title); }

  CalendarEvent *qEvent() const { return m_event; }

  QList<ClothingItem *> qPieceList() const { return m_clothingItems; }

  WeatherSnapshot *qWeather() const { return m_weather; }

  // Database ID of this recommendation, used to save choose/reject and to
  // find it in the history. -1 if it was not saved.
  qint64 qRecommendationId() const { return m_recommendationId; }

private:
  QList<ClothingItem *> m_clothingItems;
  std::string m_title;
  CalendarEvent *m_event;
  WeatherSnapshot *m_weather;
  qint64 m_recommendationId;
};

#endif // OUTFITRECOMMENDATION_H
