#include "RecommendationService.h"

#include <QDate>
#include <QHash>
#include <QTimer>
#include <QUrl>
#include <memory>
#include <QThread>

namespace {
constexpr int kRefreshTimeoutMs = 8000;
constexpr int kOutfitCount = 5;
// Used when no weather has ever been saved (first run, offline).
constexpr double kFallbackTempC = 15;
constexpr double kFallbackRain = 0;

std::optional<CalendarEntry> nextEventToday(const CalendarService &calendar) {
  const QDateTime now = QDateTime::currentDateTime();
  for (const auto &e : calendar.eventsOn(QDate::currentDate()))
    if (e.end >= now || !e.end.isValid())
      return e;
  return std::nullopt;
}

} // namespace

RecommendationService::RecommendationService(Database &database,
                                             MlClient &mlClient,
                                             ClothingRepository &clothing,
                                             WeatherService &weatherService,
                                             CalendarService &calendarService,
                                             QObject *parent)
    : QObject(parent), m_database(database), m_ml(mlClient),
      m_clothing(clothing), m_weather(weatherService),
      m_calendar(calendarService) {}

bool RecommendationService::markOutfit(qint64 recommendationId, bool chosen) {
  return m_database.execute(
      "UPDATE outfit_recommendations SET status = ? "
      "WHERE recommendation_id = ?",
      {chosen ? "chosen" : "rejected", std::to_string(recommendationId)});
}

bool RecommendationService::isChosen(qint64 recommendationId) const {
  bool found = false;
  m_database.query("SELECT 1 FROM outfit_recommendations "
                   "WHERE recommendation_id = ? AND status = 'chosen'",
                   {std::to_string(recommendationId)},
                   [&](const Database::Row &) { found = true; });
  return found;
}

qint64 RecommendationService::chosenOutfitId(qint64 eventId) const {
  qint64 id = -1;
  m_database.query(
      "SELECT o.recommendation_id FROM outfit_recommendations o "
      "JOIN recommendation_batches b ON b.batch_id = o.batch_id "
      "WHERE b.event_id = ? AND o.status = 'chosen' "
      "AND EXISTS (SELECT 1 FROM outfit_items i "
      "WHERE i.recommendation_id = o.recommendation_id) "
      "ORDER BY o.recommendation_id DESC LIMIT 1",
      {std::to_string(eventId)}, [&](const Database::Row &row) {
        id = std::stoll(row[0].value_or("-1"));
      });
  return id;
}

void RecommendationService::start(Callback done) {
  // Wait for both downloads (or the timeout), then recommend once.
  auto pending = std::make_shared<int>(2);
  auto finished = std::make_shared<bool>(false);
  auto links = std::make_shared<QList<QMetaObject::Connection>>();
  auto proceed = [this, done, finished, links]() {
    if (*finished)
      return;
    *finished = true;
    for (const auto &link : std::as_const(*links))
      disconnect(link);
    recommend(done);
  };
  auto oneDone = [pending, proceed](bool, const QString &) {
    if (--*pending == 0)
      proceed();
  };

  links->append(connect(&m_weather, &WeatherService::fetched, this, oneDone));
  links->append(connect(&m_calendar, &CalendarService::fetched, this, oneDone));
  QTimer::singleShot(kRefreshTimeoutMs, this, proceed);

  m_weather.fetch();
  m_calendar.fetch();
}

void RecommendationService::refreshWeather(Callback done) {
  connect(
      &m_weather, &WeatherService::fetched, this,
      [this, done](bool, const QString &) { recommend(done); },
      Qt::SingleShotConnection);
  m_weather.fetch();
}

void RecommendationService::regenerate(Callback done) { recommend(done); }

void RecommendationService::refresh(Callback done) { recommend(done); }

void RecommendationService::recommend(Callback done) {
  if (m_busy) {
    m_waiting = done;
    return;
  }
  Result result;
  result.closet = m_clothing.listStored();
  result.weather = m_weather.cached();
  if (m_eventId > 0)
    result.event = m_calendar.eventById(m_eventId);
  if (!result.event)
    result.event = nextEventToday(m_calendar);

  if (result.closet.empty()) {
    done(true, {}, "");
    return;
  }

  const qint64 chosenId = result.event ? chosenOutfitId(result.event->id) : -1;
  if (chosenId > 0) {
    QSet<qint64> chosenItems;
    m_database.query("SELECT clothing_item_id FROM outfit_items "
                     "WHERE recommendation_id = ?",
                     {std::to_string(chosenId)}, [&](const Database::Row &row) {
                       chosenItems.insert(std::stoll(row[0].value_or("-1")));
                     });
    QList<ClothingItem *> pieces;
    for (const auto &stored : result.closet)
      if (chosenItems.contains(stored.id))
        pieces.append(stored.item.get());
    if (!pieces.isEmpty()) {
      std::vector<OutfitRecommendation> chosen;
      chosen.emplace_back(pieces, "Outfit 1", pieces.first()->formality(),
                          nullptr, QVariantMap(), nullptr, chosenId);
      done(true,
           saveAndDecorate(chosen, {}, result.weather, result.event, chosenId),
           "");
      return;
    }
  }

  QHash<QUrl, qint64> idByImage;
  for (const auto &stored : result.closet)
    idByImage.insert(stored.item->qImageUrl(), stored.id);

  m_busy = true;
  // The ML runs on its own thread; everything after it runs on the UI thread.
  QThread *worker = QThread::create([this, closet = m_clothing.listStored(), idByImage,
               weather = result.weather, event = result.event, done]() {
    std::vector<OutfitRecommendation> outfits;
    QString error;
    try {
      outfits = m_ml.recommendLocal(closet, {}, kOutfitCount, weather, event)
                    .get();
    } catch (const std::exception &e) {
      error = QString::fromUtf8(e.what());
    }
    auto shared =
        std::make_shared<std::vector<OutfitRecommendation>>(std::move(outfits));
    QMetaObject::invokeMethod(
        this,
        [this, done, shared, idByImage, weather, event, error]() {
          if (!error.isEmpty()) {
            m_busy = false;
            done(false, {}, error);
          } else {
            auto saved = saveAndDecorate(*shared, idByImage, weather, event);
            m_busy = false;
            done(true, std::move(saved), "");
          }
          if (m_waiting) {
            Callback next = *m_waiting;
            m_waiting.reset();
            recommend(next);
          }
        },
        Qt::QueuedConnection);
  });
  connect(worker, &QThread::finished, worker, &QObject::deleteLater);
  worker->start();
}

std::vector<OutfitRecommendation> RecommendationService::saveAndDecorate(
    const std::vector<OutfitRecommendation> &outfits,
    const QHash<QUrl, qint64> &idByImage, const std::optional<WeatherData> &weather,
    const std::optional<CalendarEntry> &event, qint64 existingId) {
  for (QObject *old : std::as_const(m_owned))
    old->deleteLater();
  m_owned.clear();

  QVariantMap weatherMap;
  if (weather) {
    weatherMap["degrees_celcius"] = weather->temperatureC;
    weatherMap["cloudiness"] = weather->cloudiness;
    weatherMap["city"] = weather->location;
    weatherMap["min_daily_temp_celcius"] = weather->minTempC;
    weatherMap["max_daily_temp_celcius"] = weather->maxTempC;
    weatherMap["rain_chance"] = weather->precipitationProbability / 100.0;
  }

  CalendarEvent *eventObject = nullptr;
  if (event) {
    eventObject = new CalendarEvent(event->name, event->location, event->notes,
                                    event->start, event->end, event->dressCode,
                                    event->allDay, chosenOutfitId(event->id),
                                    this, event->id);
    m_owned.append(eventObject);
  }

  const bool saving = existingId <= 0;
  if (saving)
    m_database.execute("BEGIN");
  const bool batchSaved =
      !saving ? false :
      event ? m_database.execute(
                  "INSERT INTO recommendation_batches (event_id) VALUES (?)",
                  {std::to_string(event->id)})
            : m_database.execute(
                  "INSERT INTO recommendation_batches DEFAULT VALUES");
  const std::int64_t batchId = m_database.lastInsertId();

  std::vector<OutfitRecommendation> result;
  int rank = 0;
  for (const auto &outfit : outfits) {
    ++rank;
    qint64 recommendationId = saving ? -1 : existingId;
    if (batchSaved &&
        m_database.execute("INSERT INTO outfit_recommendations (batch_id, "
                           "rank, score) VALUES (?, ?, 0)",
                           {std::to_string(batchId), std::to_string(rank)})) {
      recommendationId = m_database.lastInsertId();
      for (ClothingItem *piece : outfit.m_clothingItems) {
        const auto id = idByImage.constFind(piece->qImageUrl());
        if (id != idByImage.constEnd())
          m_database.execute("INSERT OR IGNORE INTO outfit_items "
                             "(recommendation_id, clothing_item_id) "
                             "VALUES (?, ?)",
                             {std::to_string(recommendationId),
                              std::to_string(id.value())});
      }
    }
    QList<ClothingItem *> pieces;
    for (ClothingItem *piece : outfit.m_clothingItems) {
      auto *copy = new ClothingItem(*piece);
      copy->setParent(this);
      m_owned.append(copy);
      pieces.append(copy);
    }
    result.emplace_back(pieces, outfit.m_title,
                        outfit.m_formality, eventObject, weatherMap, nullptr,
                        recommendationId);
  }
  if (saving)
    m_database.execute("COMMIT");
  return result;
}
