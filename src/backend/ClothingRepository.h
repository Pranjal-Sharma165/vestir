#pragma once

#include "Database.h"
#include "Errors.h"
#include "PhotoStore.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class ClothingRepository {
public:
  struct Labels {
    std::string category;
    std::string subCategory;
    std::string colour;
    std::string formality;
  };

  struct MlLabels {
    std::string categoryName;
    std::string subCategory;
    std::string primaryColor;
    std::string formality;
  };

  struct Item {
    std::int64_t id = 0;
    std::string name;
    Labels labels;
    std::filesystem::path imagePath;
  };

  struct StoredItem {
    std::int64_t id = 0;
    Item item;
  };

  ClothingRepository(Database &database, PhotoStore &photoStore);

  static std::optional<Labels> labelsFromMl(const MlLabels &ml);

  std::int64_t add(const Item &item);

  void update(std::int64_t id, const Labels &labels);

  void remove(std::int64_t id);

  std::vector<StoredItem> listStored() const;

private:
  Database &m_database;
  PhotoStore &m_photoStore;
};