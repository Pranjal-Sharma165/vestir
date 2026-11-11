#include "RecommendationService.h"
#include "Database.h"
#include "EventFormality.h"

#include <QDate>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPointer>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimer>
#include <map>
#include <memory>
#include <unordered_map>

namespace {
constexpr int kRefreshTimeoutMs = 8000;
constexpr int kOutfitCount = 5;
// Used when no weather has ever been saved (first run, offline).
constexpr double kFallbackTempC = 15;
constexpr double kFallbackRain = 0;

std::optional<CalendarEntry> nextEventToday() {
  const QDateTime now = QDateTime::currentDateTime();
  for (const auto &e : CalendarService::eventsOn(QDate::currentDate()))
    if (e.end >= now || !e.end.isValid())
      return e;
  return std::nullopt;
}

QVariant eventIdOf(const std::optional<CalendarEntry> &event) {
  if (!event)
    return QVariant(QMetaType::fromType<qint64>());
  QSqlQuery q(Database::connection());
  q.prepare("SELECT event_id FROM calendar_events WHERE google_event_id = :u");
  q.bindValue(":u", event->uid);
  if (q.exec() && q.next())
    return q.value(0).toLongLong();
  return QVariant(QMetaType::fromType<qint64>());
}

QVariant weatherIdOf(const std::optional<WeatherData> &weather) {
  if (!weather)
    return QVariant(QMetaType::fromType<qint64>());
  QSqlQuery q(Database::connection());
  q.prepare("SELECT weather_id FROM weather_records "
            "WHERE location = :l AND forecast_time = :d");
  q.bindValue(":l", weather->location);
  q.bindValue(":d", weather->forecastTime);
  if (q.exec() && q.next())
    return q.value(0).toLongLong();
  return QVariant(QMetaType::fromType<qint64>());
}

// Outfits already shown today for the same event, as lists of item IDs.
QJsonArray alreadyShown(const QVariant &eventId) {
  QSqlQuery q(Database::connection());
  q.prepare("SELECT oi.recommendation_id, oi.clothing_item_id "
            "FROM outfit_items oi "
            "JOIN outfit_recommendations o "
            "  ON o.recommendation_id = oi.recommendation_id "
            "JOIN recommendation_batches b ON b.batch_id = o.batch_id "
            "WHERE date(b.created_at, 'localtime') = date('now', 'localtime') "
            "AND b.event_id IS :event ORDER BY oi.recommendation_id");
  q.bindValue(":event", eventId);
  std::map<qint64, QJsonArray> byOutfit;
  if (q.exec())
    while (q.next())
      byOutfit[q.value(0).toLongLong()].append(q.value(1).toLongLong());
  QJsonArray all;
  for (const auto &[id, items] : byOutfit)
    all.append(items);
  return all;
}

// The outfit the user chose for this event: recommendation id and item IDs.
// Empty if none (or the event is unknown).
std::pair<qint64, std::vector<qint64>> chosenFor(const QVariant &eventId) {
  if (eventId.isNull())
    return {};
  QSqlQuery q(Database::connection());
  q.prepare("SELECT o.recommendation_id FROM outfit_recommendations o "
            "JOIN recommendation_batches b ON b.batch_id = o.batch_id "
            "WHERE b.event_id = :e AND o.status = 'chosen' "
            "ORDER BY o.recommendation_id DESC LIMIT 1");
  q.bindValue(":e", eventId);
  if (!q.exec() || !q.next())
    return {};
  const qint64 id = q.value(0).toLongLong();
  std::vector<qint64> items;
  QSqlQuery it(Database::connection());
  it.prepare("SELECT clothing_item_id FROM outfit_items "
             "WHERE recommendation_id = :r");
  it.bindValue(":r", id);
  if (it.exec())
    while (it.next())
      items.push_back(it.value(0).toLongLong());
  return {id, items};
}

template <typename Outfit>
bool saveBatch(const QVariant &eventId, const QVariant &weatherId,
               std::vector<Outfit> &outfits) {
  QSqlDatabase db = Database::connection();
  if (!db.transaction())
    return false;

  QSqlQuery batch(db);
  batch.prepare("INSERT INTO recommendation_batches (event_id, weather_id) "
                "VALUES (:e, :w)");
  batch.bindValue(":e", eventId);
  batch.bindValue(":w", weatherId);
  bool ok = batch.exec();
  const qint64 batchId = ok ? batch.lastInsertId().toLongLong() : 0;

  for (auto &o : outfits) {
    if (!ok)
      break;
    QSqlQuery rec(db);
    rec.prepare("INSERT INTO outfit_recommendations (batch_id, rank, score) "
                "VALUES (:b, :r, :s)");
    rec.bindValue(":b", batchId);
    rec.bindValue(":r", o.rank);
    rec.bindValue(":s", o.score);
    ok = rec.exec();
    if (!ok)
      break;
    o.recommendationId = rec.lastInsertId().toLongLong();
    for (qint64 itemId : o.itemIds) {
      QSqlQuery link(db);
      link.prepare("INSERT INTO outfit_items (recommendation_id, "
                   "clothing_item_id) VALUES (:r, :i)");
      link.bindValue(":r", o.recommendationId);
      link.bindValue(":i", itemId);
      if (!(ok = link.exec()))
        break;
    }
  }

  if (!ok) {
    qWarning() << "Cannot save recommendations:" << db.lastError().text();
    db.rollback();
    return false;
  }
  return db.commit();
}

// Remembers the ML's event type on the calendar row, once.
void storeEventType(const std::optional<CalendarEntry> &event,
                    const QString &type) {
  if (!event || !event->eventType.isEmpty() || type.isEmpty())
    return;
  QSqlQuery q(Database::connection());
  q.prepare("UPDATE calendar_events SET event_type = :t "
            "WHERE google_event_id = :u AND (event_type IS NULL OR "
            "event_type = '')");
  q.bindValue(":t", type);
  q.bindValue(":u", event->uid);
  q.exec();
}
} // namespace

bool RecommendationService::markOutfit(qint64 recommendationId, bool chosen) {
  QSqlDatabase db = Database::connection();
  if (!db.transaction())
    return false;
  QSqlQuery status(db);
  status.prepare("UPDATE outfit_recommendations SET status = :s "
                 "WHERE recommendation_id = :r");
  status.bindValue(":s", chosen ? "chosen" : "rejected");
  status.bindValue(":r", recommendationId);
  QSqlQuery feedback(db);
  feedback.prepare("INSERT OR REPLACE INTO outfit_feedback "
                   "(recommendation_id, liked) VALUES (:r, :l)");
  feedback.bindValue(":r", recommendationId);
  feedback.bindValue(":l", chosen ? 1 : 0);
  if (!status.exec() || !feedback.exec()) {
    qWarning() << "Cannot save choice:" << db.lastError().text();
    db.rollback();
    return false;
  }
  return db.commit();
}

qint64 RecommendationService::chosenOutfitId(qint64 eventId) {
  const auto [id, items] = chosenFor(eventId);
  return items.empty() ? -1 : id;
}

bool RecommendationService::isChosen(qint64 recommendationId) {
  QSqlQuery q(Database::connection());
  q.prepare("SELECT 1 FROM outfit_recommendations "
            "WHERE recommendation_id = :r AND status = 'chosen'");
  q.bindValue(":r", recommendationId);
  return q.exec() && q.next();
}

RecommendationService::RecommendationService(QObject *parent)
    : QObject(parent), m_ml(this), m_weather(this), m_calendar(this) {}

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
    recommend(done, false);
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
      [this, done](bool, const QString &) { recommend(done, false); },
      Qt::SingleShotConnection);
  m_weather.fetch();
}

void RecommendationService::regenerate(Callback done) { recommend(done, true); }

void RecommendationService::refresh(Callback done) { recommend(done, false); }

QList<OutfitRecommendation *> RecommendationService::build(Result &&result) {
  for (QObject *old : std::as_const(m_owned))
    old->deleteLater();
  m_owned.clear();

  std::unordered_map<qint64, ClothingItem *> itemsById;
  for (auto &stored : result.closet) {
    ClothingItem *item = stored.item.release();
    item->setParent(this);
    m_owned.append(item);
    itemsById[stored.id] = item;
  }

  const OutfitFormality::Value formality = formalityForEvent({}, result.eventType);
  QList<OutfitRecommendation *> outfits;
  for (const auto &outfit : result.outfits) {
    QList<ClothingItem *> pieces;
    for (qint64 id : outfit.itemIds)
      if (auto it = itemsById.find(id); it != itemsById.end())
        pieces.append(it->second);

    WeatherSnapshot *weather = nullptr;
    if (result.weather) {
      weather = new WeatherSnapshot(*result.weather, this);
      m_owned.append(weather);
    }
    CalendarEvent *event = nullptr;
    if (result.event) {
      event = new CalendarEvent(result.event->name, result.event->location,
                                result.event->notes, result.event->start,
                                result.event->end, formality,
                                result.event->allDay, -1, this,
                                result.event->id);
      m_owned.append(event);
    }

    auto *rec = new OutfitRecommendation(
        pieces, "Outfit " + std::to_string(outfit.rank), formality, event,
        weather, this, outfit.recommendationId);
    m_owned.append(rec);
    outfits.append(rec);
  }
  return outfits;
}

void RecommendationService::recommend(Callback done, bool skipShown) {
  Result result;
  result.closet = ClothingRepository::listStored();
  result.weather = WeatherService::cached();
  if (m_eventId > 0)
    result.event = CalendarService::eventById(m_eventId);
  if (!result.event)
    result.event = nextEventToday();

  if (result.closet.empty()) {
    done(true, build(std::move(result)), {});
    return;
  }

  const QVariant eventId = eventIdOf(result.event);

  // Already decided: show the chosen outfit again, if all its items are still
  // in the closet.
  if (!skipShown) {
    const auto [chosenId, chosenItems] = chosenFor(eventId);
    bool complete = !chosenItems.empty();
    for (qint64 id : chosenItems) {
      bool found = false;
      for (const auto &s : result.closet)
        found = found || s.id == id;
      complete = complete && found;
    }
    if (complete) {
      RecommendedOutfit o;
      o.recommendationId = chosenId;
      o.rank = 1;
      o.itemIds = chosenItems;
      result.eventType = result.event->eventType;
      result.outfits.push_back(std::move(o));
      done(true, build(std::move(result)), {});
      return;
    }
  }

  QJsonArray items;
  for (const auto &s : result.closet)
    items.append(s.ml);

  QJsonObject request{
      {"temperature",
       result.weather ? result.weather->temperatureC : kFallbackTempC},
      {"rain_probability",
       result.weather ? double(result.weather->precipitationProbability)
                      : kFallbackRain},
      {"items", items},
      {"excluded_outfits",
       skipShown ? alreadyShown(eventId) : QJsonArray()},
      {"limit", kOutfitCount}};
  if (result.event)
    request["event_title"] = result.event->name;

  auto shared = std::make_shared<Result>(std::move(result));
  m_ml.recommend(
      request,
      [this, shared, eventId, done](bool ok, const QJsonObject &answer,
                              const QString &error) {
        if (!ok) {
          done(false, {}, error);
          return;
        }
        shared->eventType = answer["event"].toString();
        int rank = 0;
        for (const auto &v : answer["outfits"].toArray()) {
          RecommendedOutfit o;
          o.rank = ++rank;
          o.score = v.toObject()["score"].toDouble();
          for (const auto &id : v.toObject()["item_ids"].toArray())
            o.itemIds.push_back(id.toInteger());
          shared->outfits.push_back(std::move(o));
        }
        storeEventType(shared->event, shared->eventType);
        if (!shared->outfits.empty() &&
            !saveBatch(eventId, weatherIdOf(shared->weather),
                       shared->outfits)) {
          done(false, {}, "Could not save recommendations");
          return;
        }
        done(true, build(std::move(*shared)), {});
      });
}
