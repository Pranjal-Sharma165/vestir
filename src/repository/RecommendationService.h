#ifndef RECOMMENDATIONSERVICE_H
#define RECOMMENDATIONSERVICE_H

#include "CalendarService.h"
#include "ClothingRepository.h"
#include "MlClient.h"
#include "WeatherService.h"

#include <QObject>
#include <QString>
#include <functional>
#include <optional>
#include <vector>

/*! One recommended outfit, as saved in outfit_recommendations. */
struct RecommendedOutfit {
  qint64 recommendationId = 0;
  int rank = 0; // 1 = best
  double score = 0;
  std::vector<qint64> itemIds;
};

/*! Everything the UI needs to show one round of recommendations. */
struct RecommendationResult {
  std::vector<ClothingRepository::StoredItem> closet; // owns the items
  std::vector<RecommendedOutfit> outfits;             // best first
  std::optional<WeatherData> weather;
  std::optional<CalendarEntry> event; // today's next event, if any
  QString eventType; // school/work/party/hangout/gym, as chosen by the ML
};

/*!
 * \brief Builds the day's outfit recommendations: reads the closet, today's
 * weather and next calendar event, asks the ML (MlClient), and saves the top
 * outfits as a batch in recommendation_batches / outfit_recommendations /
 * outfit_items.
 *
 * Asking again on the same day skips outfits already shown for that event
 * ("generate again"), so the user keeps getting new ones.
 *
 * TODO later:
 *  - Weather and the event are only for today.
 *  - Mark an outfit 'chosen' / 'rejected' and store feedback when the UI has
 *    a place for it (status and outfit_feedback columns already exist).
 *  - When no outfits are left, the result is empty; the UI could say so.
 */
class RecommendationService : public QObject {
  Q_OBJECT
public:
  using Callback =
      std::function<void(bool ok, RecommendationResult &&, const QString &)>;

  explicit RecommendationService(QObject *parent = nullptr);

  /*!
   * First round for today: refreshes weather and calendar from the internet
   * (waits at most a few seconds, falls back to the saved copies), then
   * recommends. Nothing is skipped, so a restart shows the best outfits
   * again. \a done is called once.
   */
  void start(Callback done);

  /*!
   * A new round without re-downloading weather and calendar. Outfits already
   * shown today for the same event are skipped.
   */
  void regenerate(Callback done);

private:
  void recommend(Callback done, bool skipShown);

  MlClient m_ml;
  WeatherService m_weather;
  CalendarService m_calendar;
};

#endif // RECOMMENDATIONSERVICE_H
