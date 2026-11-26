#include "RecommendationService.h"
#include "TestHelpers.h"

#include <QCoreApplication>
#include <filesystem>
#include <fstream>
#include <iostream>

struct Fixture {
  std::filesystem::path dir;
  Database database;
  PhotoStore photoStore;
  ClothingRepository clothing;
  MlClient ml;
  WeatherService weather;
  CalendarService calendar;
  RecommendationService service;

  explicit Fixture(const std::string &name)
      : dir(std::filesystem::temp_directory_path() / ("vestir_test_rec_" + name)),
        database(":memory:", readSchema()), photoStore(dir),
        clothing(database, photoStore), weather(database),
        calendar(database, ml), service(database, ml, clothing, weather,
                                        calendar) {
    std::filesystem::remove_all(dir);
    database.initialize();
  }

  ~Fixture() { std::filesystem::remove_all(dir); }

  qint64 addClothing() {
    const std::filesystem::path photo = photoStore.newTempPath("jpg");
    std::ofstream(photo) << "fake image";
    ClothingItem item("Tee", ClothingCategory::Value::Top,
                      ClothingCategory::SubCategory::TShirt,
                      ClothingColour::Value::Black,
                      OutfitFormality::Value::Casual, photo);
    return clothing.add(item);
  }

  void addEvent() {
    database.execute("INSERT INTO calendar_events (event_name, start_time, "
                     "end_time, dress_code) VALUES ('Interview', "
                     "'2026-01-01 09:00:00', '2026-01-01 10:00:00', 'formal')",
                     {});
  }

  void addOutfit(const char *eventId, const char *rank, const char *status,
                 const char *itemId) {
    if (eventId)
      database.execute(
          "INSERT INTO recommendation_batches (event_id) VALUES (?)",
          {eventId});
    else
      database.execute("INSERT INTO recommendation_batches DEFAULT VALUES", {});
    const std::string batch = std::to_string(database.lastInsertId());
    database.execute("INSERT INTO outfit_recommendations (batch_id, rank, "
                     "score, status) VALUES (?, ?, 0, ?)",
                     {batch, rank, status});
    if (itemId)
      database.execute("INSERT INTO outfit_items VALUES (?, ?)",
                       {std::to_string(database.lastInsertId()), itemId});
  }
};

static void testMarkOutfit() {
  Fixture f("mark");
  f.addClothing();
  f.addOutfit(nullptr, "1", "shown", "1");

  check(!f.service.isChosen(1), "a new outfit is not chosen");
  check(f.service.markOutfit(1, true), "markOutfit works");
  check(f.service.isChosen(1), "chosen outfit is chosen");
  f.service.markOutfit(1, false);
  check(!f.service.isChosen(1), "rejected outfit is not chosen");
  check(!f.service.isChosen(999), "unknown outfit is not chosen");
}

static void testChosenOutfitId() {
  Fixture f("chosen");
  f.addClothing();
  f.addEvent();
  check(f.service.chosenOutfitId(1) == -1, "no outfit chosen at first");

  f.addOutfit("1", "1", "shown", "1");
  check(f.service.chosenOutfitId(1) == -1, "a shown outfit does not count");

  f.service.markOutfit(1, true);
  check(f.service.chosenOutfitId(1) == 1, "chosen outfit is found for event");
  check(f.service.chosenOutfitId(2) == -1, "other events are not affected");

  f.addOutfit("1", "1", "chosen", nullptr);
  check(f.service.chosenOutfitId(1) == 1,
        "a chosen outfit without items is ignored");
}

static void testEmptyWardrobe() {
  Fixture f("empty");
  bool called = false, ok = false;
  size_t count = 99;
  f.service.regenerate([&](bool success, const auto &outfits, const QString &) {
    called = true;
    ok = success;
    count = outfits.size();
  });
  check(called, "regenerate answers right away with no clothes");
  check(ok && count == 0, "an empty wardrobe gives no outfits and no error");
}

static void testChosenOutfitIsReused() {
  Fixture f("reuse");
  f.addClothing();
  f.addEvent();
  f.addOutfit("1", "1", "chosen", "1");
  f.service.setEvent(1);

  bool called = false, ok = false;
  std::vector<OutfitRecommendation> result;
  f.service.regenerate(
      [&](bool success, const std::vector<OutfitRecommendation> outfits,
          const QString &) {
        called = true;
        ok = success;
        result = outfits;
      });
  check(called && ok, "regenerate answers without the ML");
  check(result.size() == 1, "the chosen outfit is returned alone");
  if (result.size() != 1)
    return;
  check(result[0].m_recommendationId == 1, "it is the chosen outfit");
  check(result[0].m_clothingItems.size() == 1, "it has its clothing item");
  check(result[0].m_event && result[0].m_event->qName() == "Interview",
        "it is linked to the event");
}

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  testMarkOutfit();
  testChosenOutfitId();
  testEmptyWardrobe();
  testChosenOutfitIsReused();
  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
