#pragma once

#include <string>
#include <sqlite3.h>

class Database {
public:
    Database(const std::string& databaseFile,
             const std::string& schemaSql);

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

private:
    bool open(); // Handle SQLite connection
    bool createSchema();
    bool execute(const std::string& sql);
    void close();

    std::string m_databaseFile;
    std::string m_schemaSql;
    sqlite3* m_connection = nullptr;
};