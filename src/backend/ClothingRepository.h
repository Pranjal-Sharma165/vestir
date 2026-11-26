#pragma once

#include "../model/ClothingItem.h"
#include "Database.h"
#include "Errors.h"
#include "PhotoStore.h"

#include <QJsonObject>
#include <QtTypes>
#include <memory>
#include <optional>
#include <vector>

class ClothingRepository {
public:
  struct Labels {
    ClothingCategory::Value category{};
    ClothingCategory::SubCategory subCategory{};
    ClothingColour::Value colour{};
    OutfitFormality::Value formality{};
  };

  struct StoredItem {
    qint64 id = 0;
    std::unique_ptr<ClothingItem> item;
    QJsonObject ml;
  };

  ClothingRepository(Database &database, PhotoStore &photoStore);

  qint64 add(const ClothingItem &item);
  void update(qint64 id, const Labels &labels);
  void remove(qint64 id);

  std::vector<StoredItem> listStored() const;

private:
  Database &m_database;
  PhotoStore &m_photoStore;
};