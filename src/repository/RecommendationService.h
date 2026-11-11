#ifndef RECOMMENDATIONSERVICE_H
#define RECOMMENDATIONSERVICE_H

#include "CalendarService.h"
#include "ClothingRepository.h"
#include "MlClient.h"
#include "OutfitRecommendation.h"
#include "WeatherService.h"

#include <QList>
#include <QObject>
#include <QString>
#include <functional>
#include <optional>
#include <vector>

/*!
 * \brief Builds the day's outfit recommendations: reads the closet, today's
 * weather and next calendar event, asks the ML (MlClient), and saves the top
 * outfits as a batch in recommendation_batches / outfit_recommendations /
 * outfit_items.
 *
 * An event that already has a chosen outfit shows that outfit again.
 * Asking again on the same day skips outfits already shown for that event
 * ("generate again"), so the user keeps getting new ones.
 *
 * Results are the model classes (OutfitRecommendation with its ClothingItem,
 * CalendarEvent and WeatherSnapshot), best outfit first. The service owns
 * them: they stay valid until the next round is delivered.
 *
 * TODO later:
 *  - Weather and the event are only for today.
 *  - When no outfits are left, the result is empty; the UI could say so.
 */
class RecommendationService : public QObject {
  Q_OBJECT
public:
  /*!
   * Called once with the outfits (empty if the closet cannot make any), or
   * with ok = false and an error message.
   */
  using Callback = std::function<void(
      bool ok, const QList<OutfitRecommendation *> &outfits,
      const QString &error)>;

  explicit RecommendationService(QObject *parent = nullptr);

  /*!
   * Makes the next rounds for this calendar event (a calendar_events id)
   * instead of today's next event. -1 goes back to the default. Call
   * refresh() afterwards to get the outfits.
   */
  void setEvent(qint64 eventId) { m_eventId = eventId; }

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

  /*!
   * A fresh round after the closet changed: no download, and nothing is
   * skipped, because new items make new outfits possible.
   */
  void refresh(Callback done);

  /*!
   * Records the user's decision on a shown outfit: status becomes 'chosen'
   * or 'rejected', and a liked/disliked row is saved in outfit_feedback.
   */
  static bool markOutfit(qint64 recommendationId, bool chosen);

  /*! Recommendation id of the outfit chosen for this event, or -1. */
  static qint64 chosenOutfitId(qint64 eventId);

  /*! True if the outfit was chosen (status 'chosen'). */
  static bool isChosen(qint64 recommendationId);

private:
  /*! One recommended outfit, as saved in outfit_recommendations. */
  struct RecommendedOutfit {
    qint64 recommendationId = 0;
    int rank = 0; // 1 = best
    double score = 0;
    std::vector<qint64> itemIds;
  };

  /*! Everything one round produced, before it becomes model objects. */
  struct Result {
    std::vector<ClothingRepository::StoredItem> closet; // owns the items
    std::vector<RecommendedOutfit> outfits;             // best first
    std::optional<WeatherData> weather;
    std::optional<CalendarEntry> event; // today's next event, if any
    QString eventType; // school/work/party/hangout/gym, as chosen by the ML
  };

  void recommend(Callback done, bool skipShown);
  QList<OutfitRecommendation *> build(Result &&result);

  qint64 m_eventId = -1; // chosen event, or -1 for today's next one
  QList<QObject *> m_owned; // everything created for the current round

  MlClient m_ml;
  WeatherService m_weather;
  CalendarService m_calendar;
};

#endif // RECOMMENDATIONSERVICE_H
