#ifndef RECOMMENDATIONSERVICE_H
#define RECOMMENDATIONSERVICE_H

#include "CalendarService.h"
#include "ClothingRepository.h"
#include "Database.h"
#include "MlClient.h"
#include "OutfitRecommendation.h"
#include "WeatherService.h"

#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QUrl>
#include <QObject>
#include <QString>
#include <functional>
#include <optional>
#include <vector>

class RecommendationService : public QObject {
  Q_OBJECT
public:
  using Callback = std::function<void(
      bool ok, const std::vector<OutfitRecommendation> outfits,
      const QString &error)>;

  explicit RecommendationService(Database &database, MlClient &mlClient,
                                 ClothingRepository &clothing,
                                 WeatherService &weatherService,
                                 CalendarService &calendarService,
                                 QObject *parent = nullptr);
  void setEvent(qint64 eventId) { m_eventId = eventId; }
  void start(Callback done);
  void regenerate(Callback done);
  void refresh(Callback done);
  void refreshWeather(Callback done);
  bool markOutfit(qint64 recommendationId, bool chosen);
  bool isChosen(qint64 recommendationId) const;
  qint64 chosenOutfitId(qint64 eventId) const;

private:
  struct RecommendedOutfit {
    qint64 recommendationId = 0;
    int rank = 0; // 1 = best
    double score = 0;
    std::vector<qint64> itemIds;
  };
  struct Result {
    std::vector<ClothingRepository::StoredItem> closet;
    std::vector<RecommendedOutfit> outfits;
    OutfitFormality::Value formality;
    std::optional<WeatherData> weather;
    std::optional<CalendarEntry> event;
  };

  void recommend(Callback done);
  std::vector<OutfitRecommendation>
  saveAndDecorate(const std::vector<OutfitRecommendation> &outfits,
                  const QHash<QUrl, qint64> &idByImage,
                  const std::optional<WeatherData> &weather,
                  const std::optional<CalendarEntry> &event,
                  qint64 existingId = -1);

  qint64 m_eventId = -1;
  QList<QObject *> m_owned;
  bool m_busy = false;
  std::optional<Callback> m_waiting;

  Database &m_database;
  MlClient &m_ml;
  ClothingRepository &m_clothing;
  WeatherService &m_weather;
  CalendarService &m_calendar;
};

#endif // RECOMMENDATIONSERVICE_H
