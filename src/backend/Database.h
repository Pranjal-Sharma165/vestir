#pragma once

#include <sqlite3.h>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

class Database {
public:
    using Row = std::vector<std::optional<std::string>>;
    using QueryCallback = std::function<void(const Row &)>;

    Database(const std::string &databaseFile, const std::string &schemaSql);

    ~Database();

    // Prevent copying the database
    Database(const Database&) = delete; // block Database copy = original;
    Database& operator=(const Database&) = delete; // block existing = original;

    // Open or create the database
    bool initialize();

    // Return the SQLite connection
    sqlite3* connection() const;

    // Read a saved app setting
    std::string setting(const std::string& key) const;

    // Save or update an app setting
    bool setSetting(const std::string& key,
                    const std::string& value);

    // Execute a statement with text parameters
    bool execute(const std::string &sql,
                 const std::vector<std::string> &parameters = {});

    // Run a query and pass each row as optional text values
    bool query(const std::string &sql,
               const std::vector<std::string> &parameters,
               const QueryCallback &callback) const;

    std::int64_t lastInsertId() const;
    int changes() const;

private:
    bool open();
    bool createSchema();
    void close();

    std::string m_databaseFile;
    std::string m_schemaSql;
    sqlite3* m_connection = nullptr;
};