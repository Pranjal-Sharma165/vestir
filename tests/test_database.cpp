#include "Database.h"
#include "TestHelpers.h"

#include <vector>

static std::vector<std::string> tableNames(const Database &database) {
  std::vector<std::string> names;
  database.query("SELECT name FROM sqlite_master WHERE type = 'table'", {},
                 [&](const Database::Row &row) { names.push_back(*row[0]); });
  return names;
}

static bool hasTable(const std::vector<std::string> &names,
                     const std::string &name) {
  for (const std::string &existing : names) {
    if (existing == name)
      return true;
  }
  return false;
}

static void testSchemaCreatesTables() {
  Database database(":memory:", readSchema());
  check(database.initialize(), "initialize opens an in-memory database");

  const std::vector<std::string> names = tableNames(database);
  check(hasTable(names, "clothing_items"), "schema creates clothing_items");
  check(hasTable(names, "calendar_events"), "schema creates calendar_events");
  check(hasTable(names, "weather_records"), "schema creates weather_records");
  check(hasTable(names, "recommendation_batches"),
        "schema creates recommendation_batches");
  check(hasTable(names, "outfit_recommendations"),
        "schema creates outfit_recommendations");
  check(hasTable(names, "outfit_items"), "schema creates outfit_items");
  check(hasTable(names, "app_settings"), "schema creates app_settings");
}

static void testSettings() {
  Database database(":memory:", readSchema());
  database.initialize();

  check(database.setting("city").empty(), "missing setting is empty");

  check(database.setSetting("city", "Vancouver"), "setSetting saves a value");
  check(database.setting("city") == "Vancouver", "setting returns saved value");

  database.setSetting("city", "Burnaby");
  check(database.setting("city") == "Burnaby", "setSetting updates a value");
}

static void testExecuteAndQueryWithParameters() {
  Database database(":memory:", readSchema());
  database.initialize();

  check(database.execute("INSERT INTO app_settings (key, value) VALUES (?, ?)",
                         {"theme", "dark"}),
        "execute inserts a row with parameters");

  std::vector<std::string> values;
  const bool ok = database.query(
      "SELECT value FROM app_settings WHERE key = ?", {"theme"},
      [&](const Database::Row &row) { values.push_back(*row[0]); });

  check(ok, "query succeeds");
  check(values.size() == 1 && values[0] == "dark",
        "query returns the inserted row");

  check(!database.execute("INSERT INTO missing_table VALUES (1)"),
        "execute fails for a bad statement");
}

static void testNullColumnsAreEmpty() {
  Database database(":memory:", readSchema());
  database.initialize();
  database.execute("INSERT INTO app_settings (key, value) VALUES ('empty', NULL)");

  bool isNull = false;
  database.query("SELECT value FROM app_settings WHERE key = 'empty'", {},
                 [&](const Database::Row &row) { isNull = !row[0].has_value(); });

  check(isNull, "NULL column comes back as no value");
}

static void testLastInsertIdAndChanges() {
  Database database(":memory:", readSchema());
  database.initialize();

  database.execute("INSERT INTO app_settings (key, value) VALUES ('a', '1')");
  const std::int64_t first = database.lastInsertId();
  database.execute("INSERT INTO app_settings (key, value) VALUES ('b', '2')");
  check(database.lastInsertId() == first + 1,
        "lastInsertId increases after another insert");

  database.execute("UPDATE app_settings SET value = '3' WHERE key = 'a'");
  check(database.changes() == 1, "changes counts one updated row");

  database.execute("DELETE FROM app_settings WHERE key = 'missing'");
  check(database.changes() == 0, "changes is 0 when nothing matches");
}

static void testNoConnectionBeforeInitialize() {
  Database database(":memory:", readSchema());
  check(!database.execute("SELECT 1"), "execute fails before initialize");
  check(database.setting("city").empty(), "setting is empty before initialize");
}

int main() {
  testSchemaCreatesTables();
  testSettings();
  testExecuteAndQueryWithParameters();
  testNullColumnsAreEmpty();
  testLastInsertIdAndChanges();
  testNoConnectionBeforeInitialize();

  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
