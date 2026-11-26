#include "Database.h"

#include <filesystem>
#include <sqlite3.h>

Database::Database(const std::string &databaseFile,
                   const std::string &schemaSql)
    : m_databaseFile(databaseFile),
      m_schemaSql(schemaSql) {}

Database::~Database() {
  close();
}

bool Database::initialize() {
  if (m_connection != nullptr)
    return true;

  const std::filesystem::path databasePath(m_databaseFile);
  const std::filesystem::path parentDirectory = databasePath.parent_path();

  if (!parentDirectory.empty()) {
    std::error_code error;
    std::filesystem::create_directories(parentDirectory, error);

    if (error)
      return false;
  }

  const bool databaseAlreadyExists = std::filesystem::exists(databasePath);

  if (!open())
    return false;

  // Enable foreign keys
  if (!execute("PRAGMA foreign_keys = ON;")) {
    close();
    return false;
  }

  if (!databaseAlreadyExists && !createSchema()) {
    close();

    std::error_code error;
    std::filesystem::remove(databasePath, error);

    return false;
  }

  return true;
}

sqlite3 *Database::connection() const {
  return m_connection;
}

bool Database::open() {
  // Open the database file, or create it if it does not exist
  const int result =
        sqlite3_open_v2(m_databaseFile.c_str(), &m_connection,
                        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);

  if (result != SQLITE_OK) {
      close();
      return false;
  }

  return true;
}

bool Database::createSchema() {
  // Run the schema SQL that was given to the constructor
  return execute(m_schemaSql);
}

bool Database::execute(const std::string &sql,
                         const std::vector<std::string> &parameters) {
  if (m_connection == nullptr)
      return false;

  if (parameters.empty()) {
      char *errorMessage = nullptr;
      const int result =
          sqlite3_exec(m_connection, sql.c_str(), nullptr, nullptr,
                       &errorMessage);
      if (errorMessage != nullptr)
        sqlite3_free(errorMessage);
      return result == SQLITE_OK;
  }

  sqlite3_stmt *statement = nullptr;
  if (sqlite3_prepare_v2(m_connection, sql.c_str(), -1, &statement, nullptr) !=
        SQLITE_OK)
      return false;

  for (std::size_t index = 0; index < parameters.size(); ++index) {
      if (sqlite3_bind_text(statement, static_cast<int>(index + 1),
                            parameters[index].c_str(), -1,
                            SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
      }
  }

  const bool success = sqlite3_step(statement) == SQLITE_DONE;
  sqlite3_finalize(statement);
  return success;
}

std::string Database::setting(const std::string &key) const {
  if (m_connection == nullptr)
      return {};

  const char* sql =
      "SELECT value "
      "FROM app_settings "
      "WHERE key = ?;";

  sqlite3_stmt *statement = nullptr;

  // Compile the SQL query
  if (sqlite3_prepare_v2(
          m_connection,
          sql,
          -1,
          &statement,
          nullptr) != SQLITE_OK) {
      return {};
  }

  // Replace ? with the setting key
  sqlite3_bind_text(
      statement,
      1,
      key.c_str(),
      -1,
      SQLITE_TRANSIENT);

  std::string value;

  // Read the matching setting and convert it to a C++ string
  if (sqlite3_step(statement) == SQLITE_ROW) {
      const unsigned char* result = sqlite3_column_text(statement, 0);

      if (result != nullptr)
          value = reinterpret_cast<const char*>(result);
  }

  // Release the prepared statement
  sqlite3_finalize(statement);

  return value;
}

bool Database::setSetting(const std::string &key,
                          const std::string &value) {
  if (m_connection == nullptr)
      return false;

  // Insert a new setting or update
  const char* sql =
      "INSERT INTO app_settings (key, value) "
      "VALUES (?, ?) "
      "ON CONFLICT(key) "
      "DO UPDATE SET value = excluded.value;";

  sqlite3_stmt *statement = nullptr;

  // Compile the SQL query
  if (sqlite3_prepare_v2(
          m_connection,
          sql,
          -1,
          &statement,
          nullptr) != SQLITE_OK) {
      return false;
  }

  // Replace the first ? with the setting key
  sqlite3_bind_text(
      statement,
      1,
      key.c_str(),
      -1,
      SQLITE_TRANSIENT);

  // Replace the second ? with the setting value
  sqlite3_bind_text(
      statement,
      2,
      value.c_str(),
      -1,
      SQLITE_TRANSIENT);

  // Execute the INSERT or UPDATE
  const bool success =
      sqlite3_step(statement) == SQLITE_DONE;

  // Release the prepared statement
  sqlite3_finalize(statement);

  return success;
}

void Database::close() {
  if (m_connection != nullptr) {
    sqlite3_close(m_connection);
    m_connection = nullptr;
  }
}

bool Database::query(const std::string &sql,
                       const std::vector<std::string> &parameters,
                       const QueryCallback &callback) const {
  if (m_connection == nullptr)
      return false;

  sqlite3_stmt *statement = nullptr;
  if (sqlite3_prepare_v2(m_connection, sql.c_str(), -1, &statement, nullptr) !=
        SQLITE_OK)
      return false;

  for (std::size_t index = 0; index < parameters.size(); ++index) {
      if (sqlite3_bind_text(statement, static_cast<int>(index + 1),
                            parameters[index].c_str(), -1,
                            SQLITE_TRANSIENT) != SQLITE_OK) {
        sqlite3_finalize(statement);
        return false;
      }
  }

  const int columnCount = sqlite3_column_count(statement);
  int result = SQLITE_OK;
  while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
      Row row;
      row.reserve(static_cast<std::size_t>(columnCount));
      for (int column = 0; column < columnCount; ++column) {
        const unsigned char *value = sqlite3_column_text(statement, column);
        if (value == nullptr)
          row.emplace_back(std::nullopt);
        else
          row.emplace_back(reinterpret_cast<const char *>(value));
      }
      callback(row);
  }

  sqlite3_finalize(statement);
  return result == SQLITE_DONE;
}

std::int64_t Database::lastInsertId() const {
  return m_connection == nullptr ? 0 : sqlite3_last_insert_rowid(m_connection);
}

int Database::changes() const {
  return m_connection == nullptr ? 0 : sqlite3_changes(m_connection);
}