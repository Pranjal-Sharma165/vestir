#include "HistoryRepository.h"
#include "Database.h"

#include <QMap>
#include <QSqlQuery>
#include <QTimeZone>

namespace {
const char *kStoreFormat = "yyyy-MM-dd HH:mm:ss";
}

std::vector<HistoryEntry>
HistoryRepository::chosenOutfits(const QDateTime &from, const QDateTime &to) {
  QSqlQuery q(Database::connection());
  q.prepare("SELECT o.recommendation_id, o.recommended_at, e.event_name, "
            "e.event_type FROM outfit_recommendations o "
            "JOIN recommendation_batches b ON b.batch_id = o.batch_id "
            "LEFT JOIN calendar_events e ON e.event_id = b.event_id "
            "WHERE o.status = 'chosen' AND o.recommended_at >= :from "
            "AND o.recommended_at <= :to "
            "ORDER BY o.recommended_at DESC, o.recommendation_id DESC");
  q.bindValue(":from", from.toUTC().toString(kStoreFormat));
  q.bindValue(":to", to.toUTC().toString(kStoreFormat));
  if (!q.exec())
    throw DatabaseError("Cannot read the outfit history");

  std::vector<HistoryEntry> out;
  while (q.next()) {
    HistoryEntry e;
    e.recommendationId = q.value(0).toLongLong();
    QDateTime when =
        QDateTime::fromString(q.value(1).toString(), kStoreFormat);
    when.setTimeZone(QTimeZone::UTC);
    e.chosenAt = when.toLocalTime();
    e.eventName = q.value(2).toString();
    e.eventType = q.value(3).toString();
    out.push_back(e);
  }

  for (HistoryEntry &e : out) {
    QSqlQuery items(Database::connection());
    items.prepare("SELECT clothing_item_id FROM outfit_items "
                  "WHERE recommendation_id = :id ORDER BY clothing_item_id");
    items.bindValue(":id", e.recommendationId);
    if (!items.exec())
      throw DatabaseError("Cannot read the outfit history");
    while (items.next())
      e.itemIds.push_back(items.value(0).toLongLong());
  }
  return out;
}
