#include "HistoryRepository.h"
#include "TestHelpers.h"

#include <iostream>
static void addItem(Database &database, const char *name) {
  database.execute("INSERT INTO clothing_items (category_id, item_name, "
                   "image_path) VALUES (1, ?, 'x.jpg')",
                   {name});
}

static void addBatch(Database &database, const char *eventId) {
  if (eventId) {
    database.execute("INSERT INTO calendar_events (event_id, event_name, "
                     "start_time) VALUES (?, 'Interview', '2026-01-01 09:00')",
                     {eventId});
    database.execute("INSERT INTO recommendation_batches (event_id) VALUES (?)",
                     {eventId});
  } else {
    database.execute("INSERT INTO recommendation_batches DEFAULT VALUES", {});
  }
}

static void addOutfit(Database &database, const char *batchId, const char *rank,
                      const char *status, const char *time,
                      std::initializer_list<const char *> itemIds) {
  database.execute("INSERT INTO outfit_recommendations (batch_id, rank, "
                   "score, status, recommended_at) VALUES (?, ?, 1.0, ?, ?)",
                   {batchId, rank, status, time});
  const std::string id = std::to_string(database.lastInsertId());
  for (const char *itemId : itemIds)
    database.execute("INSERT INTO outfit_items VALUES (?, ?)", {id, itemId});
}

static void testEmptyHistory() {
  Database database(":memory:", readSchema());
  database.initialize();
  HistoryRepository history(database);
  check(history.chosenOutfits().empty(), "new database has no history");
}

static void testChosenOutfitHasItemsAndEvent() {
  Database database(":memory:", readSchema());
  database.initialize();
  addItem(database, "shirt");
  addItem(database, "jeans");
  addBatch(database, "1");
  addOutfit(database, "1", "1", "chosen", "2026-01-01 08:00:00", {"2", "1"});

  HistoryRepository history(database);
  const auto entries = history.chosenOutfits();
  check(entries.size() == 1, "one chosen outfit is listed");
  if (entries.size() != 1)
    return;
  check(entries[0].eventName == "Interview", "entry has the event name");
  check(entries[0].chosenAtUtc == "2026-01-01 08:00:00",
        "entry has the time");
  check(entries[0].itemIds.size() == 2, "entry has both items");
  check(entries[0].itemIds[0] == 1 && entries[0].itemIds[1] == 2,
        "item ids are sorted");
}

static void testOutfitWithoutEvent() {
  Database database(":memory:", readSchema());
  database.initialize();
  addItem(database, "shirt");
  addBatch(database, nullptr);
  addOutfit(database, "1", "1", "chosen", "2026-01-01 08:00:00", {"1"});

  HistoryRepository history(database);
  const auto entries = history.chosenOutfits();
  check(entries.size() == 1, "outfit without event is listed");
  check(!entries.empty() && entries[0].eventName.empty(),
        "event name is empty without an event");
}

static void testOnlyChosenListed() {
  Database database(":memory:", readSchema());
  database.initialize();
  addItem(database, "shirt");
  addBatch(database, nullptr);
  addOutfit(database, "1", "1", "shown", "2026-01-01 08:00:00", {"1"});
  addOutfit(database, "1", "2", "rejected", "2026-01-01 08:00:00", {"1"});
  addOutfit(database, "1", "3", "chosen", "2026-01-01 08:00:00", {"1"});

  HistoryRepository history(database);
  const auto entries = history.chosenOutfits();
  check(entries.size() == 1, "shown and rejected outfits are left out");
  check(!entries.empty() && entries[0].recommendationId == 3,
        "only the chosen outfit is returned");
}

static void testNewestFirst() {
  Database database(":memory:", readSchema());
  database.initialize();
  addItem(database, "shirt");
  addBatch(database, nullptr);
  addOutfit(database, "1", "1", "chosen", "2026-01-01 08:00:00", {"1"});
  addOutfit(database, "1", "2", "chosen", "2026-01-03 08:00:00", {"1"});
  addOutfit(database, "1", "3", "chosen", "2026-01-02 08:00:00", {"1"});

  HistoryRepository history(database);
  const auto entries = history.chosenOutfits();
  check(entries.size() == 3, "all chosen outfits are listed");
  if (entries.size() != 3)
    return;
  check(entries[0].recommendationId == 2 && entries[1].recommendationId == 3 &&
            entries[2].recommendationId == 1,
        "history is newest first");
}

int main() {
  testEmptyHistory();
  testChosenOutfitHasItemsAndEvent();
  testOutfitWithoutEvent();
  testOnlyChosenListed();
  testNewestFirst();
  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
