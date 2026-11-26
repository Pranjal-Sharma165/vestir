#pragma once

#include "Database.h"
#include "Errors.h"

#include <cstdint>
#include <string>
#include <vector>

struct HistoryEntry {
  std::int64_t recommendationId = 0;
  std::string chosenAtUtc; // yyyy-MM-dd HH:mm:ss
  std::string eventName;
  std::vector<std::int64_t> itemIds;
};

// Reads the outfits the user has chosen
class HistoryRepository {
public:
  explicit HistoryRepository(Database &database);

  std::vector<HistoryEntry> chosenOutfits() const;

private:
  Database &m_database;
};
