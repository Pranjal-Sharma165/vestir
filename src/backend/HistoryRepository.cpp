#include "HistoryRepository.h"

namespace {

std::string valueOrEmpty(const Database::Row &row, std::size_t index) {
  if (index >= row.size() || !row[index])
    return {};
  return *row[index];
}

} // namespace

HistoryRepository::HistoryRepository(Database &database)
    : m_database(database) {}

std::vector<HistoryEntry> HistoryRepository::chosenOutfits() const {
  std::vector<HistoryEntry> out;

  if (!m_database.query(
          "SELECT o.recommendation_id, o.recommended_at, e.event_name "
          "FROM outfit_recommendations o "
          "JOIN recommendation_batches b ON b.batch_id = o.batch_id "
          "LEFT JOIN calendar_events e ON e.event_id = b.event_id "
          "WHERE o.status = 'chosen' "
          "ORDER BY o.recommended_at DESC, o.recommendation_id DESC",
          {}, [&](const Database::Row &row) {
            HistoryEntry entry;
            entry.recommendationId = std::stoll(valueOrEmpty(row, 0));
            entry.chosenAtUtc = valueOrEmpty(row, 1);
            entry.eventName = valueOrEmpty(row, 2);
            out.push_back(std::move(entry));
          }))
    throw DatabaseError("Cannot read the outfit history");

  for (HistoryEntry &entry : out) {
    if (!m_database.query(
            "SELECT clothing_item_id FROM outfit_items "
            "WHERE recommendation_id = ? ORDER BY clothing_item_id",
            {std::to_string(entry.recommendationId)},
            [&](const Database::Row &row) {
              entry.itemIds.push_back(std::stoll(valueOrEmpty(row, 0)));
            }))
      throw DatabaseError("Cannot read the outfit history");
  }
  return out;
}
