#include "Database.h"

#include <fstream>
#include <iterator>
#include <sqlite3.h>

using namespace std;

Database::Database(const filesystem::path& databaseFile,
                   const filesystem::path& schemaFile)
    : m_databaseFile(databaseFile),
      m_schemaFile(schemaFile) {}

Database::~Database() {
  close();
}

bool Database::initialize() {
  if (m_connection != nullptr)
      return true;

  const filesystem::path parentDirectory = m_databaseFile.parent_path();

  if (!parentDirectory.empty()) {
      error_code error;
      filesystem::create_directories(parentDirectory, error);

      if (error)
          return false;
  }

  const bool databaseAlreadyExists = filesystem::exists(m_databaseFile);

  if (!open())
      return false;

  // Enable foreign keys
  if (!execute("PRAGMA foreign_keys = ON;")) {
      close();
      return false;
  }

  if (!databaseAlreadyExists && !createSchema()) {
      close();

      error_code error;
      filesystem::remove(m_databaseFile, error);

      return false;
  }

  return true;
}

sqlite3* Database::connection() const {
  return m_connection;
}

bool Database::open() {
  // Open the database file, or create it if it does not exist
  const int result = sqlite3_open_v2(
      m_databaseFile.string().c_str(),
      &m_connection,
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
      nullptr);

  if (result != SQLITE_OK) {
      close();
      return false;
  }

  return true;
}

bool Database::createSchema() {
  // Open the SQL schema file for reading
  ifstream schemaFile(m_schemaFile);

  if (!schemaFile)
      return false;

  // Read the complete schema file into one string
  const string schema{
      istreambuf_iterator<char>(schemaFile),
      istreambuf_iterator<char>()
    };

  // Execute the SQL script
  return execute(schema);
}

bool Database::execute(const string& sql) {
  if (m_connection == nullptr)
      return false;

  char* errorMessage = nullptr;

  // Execute the SQL script on the current database connection
  const int result = sqlite3_exec(
      m_connection,
      sql.c_str(),
      nullptr,
      nullptr,
      &errorMessage);

  if (errorMessage != nullptr)
      sqlite3_free(errorMessage);

  // SQL executed successfully
  return result == SQLITE_OK;
}

string Database::setting(const string& key) const {
  if (m_connection == nullptr)
      return {};

  const char* sql =
      "SELECT value "
      "FROM app_settings "
      "WHERE key = ?;";

  sqlite3_stmt* statement = nullptr;

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

  string value;

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

bool Database::setSetting(const string& key,
                          const string& value) {
  if (m_connection == nullptr)
      return false;

  // Insert a new setting or update
  const char* sql =
      "INSERT INTO app_settings (key, value) "
      "VALUES (?, ?) "
      "ON CONFLICT(key) "
      "DO UPDATE SET value = excluded.value;";

  sqlite3_stmt* statement = nullptr;

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