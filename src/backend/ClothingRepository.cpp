#include "ClothingRepository.h"

#include "EnumStringMappings.h"

#include <QDebug>
#include <QString>
#include <algorithm>
#include <filesystem>
#include <optional>

namespace {

std::string valueOrEmpty(const Database::Row &row, std::size_t index) {
  if (index >= row.size() || !row[index])
    return {};
  return *row[index];
}

template <typename E, std::size_t N>
std::string toText(const std::array<Label<E>, N> &table, E value) {
  const auto text = textFor(table, value);
  return text ? std::string(*text) : std::string();
}

template <typename E, std::size_t N>
std::optional<E> fromText(const std::array<Label<E>, N> &table,
                          const std::string &text) {
  return valueFor(table, text);
}

} // namespace

ClothingRepository::ClothingRepository(Database &database,
                                       PhotoStore &photoStore)
    : m_database(database), m_photoStore(photoStore) {}

qint64 ClothingRepository::add(const ClothingItem &item) {
  const std::filesystem::path tempPath = item.m_imageUrl;
  const std::filesystem::path storedPath =
      m_photoStore.addPhoto(tempPath, false);
  if (storedPath.empty())
    return 0;

  const bool saved = m_database.execute(
      "INSERT INTO clothing_items "
      "(category_id, item_name, image_path, sub_category, primary_color, "
      "formality) VALUES ("
      "(SELECT category_id FROM clothing_categories WHERE category_name = ?), "
      "?, ?, ?, ?, ?)",
      {toText(kCategories, item.category()), item.name(), storedPath.string(),
       toText(kSubCategories, item.subCategory()),
       toText(kColours, item.colour()),
       toText(kFormalities, item.formality())});

  if (!saved) {
    qWarning() << "Cannot save clothing item";
    std::error_code error;
    std::filesystem::remove(m_photoStore.absoluteImagePath(storedPath), error);
    return 0;
  }
  m_photoStore.discardPhoto(tempPath);
  return m_database.lastInsertId();
}

void ClothingRepository::update(qint64 id, const Labels &labels) {
  if (!ClothingCategory::subcategoryIsInCategory(labels.subCategory,
                                                 labels.category))
    throw InvalidInputError(
        "That sub-category does not belong to the category");

  const bool saved = m_database.execute(
      "UPDATE clothing_items SET "
      "category_id = (SELECT category_id FROM clothing_categories "
      "WHERE category_name = ?), "
      "sub_category = ?, primary_color = ?, formality = ? "
      "WHERE clothing_item_id = ?",
      {toText(kCategories, labels.category),
       toText(kSubCategories, labels.subCategory),
       toText(kColours, labels.colour), toText(kFormalities, labels.formality),
       std::to_string(id)});
  if (!saved)
    throw DatabaseError("Cannot save the changes");
  if (m_database.changes() == 0)
    throw NotFoundError("This clothing item no longer exists");
}

void ClothingRepository::remove(qint64 id) {
  std::string photo;
  bool found = false;
  if (!m_database.query(
          "SELECT image_path FROM clothing_items WHERE clothing_item_id = ?",
          {std::to_string(id)}, [&](const Database::Row &row) {
            photo = valueOrEmpty(row, 0);
            found = true;
          }))
    throw DatabaseError("Cannot delete the item");
  if (!found)
    throw NotFoundError("This clothing item no longer exists");

  if (!m_database.execute(
          "DELETE FROM clothing_items WHERE clothing_item_id = ?",
          {std::to_string(id)}))
    throw DatabaseError("Cannot delete the item");
  if (!photo.empty()) {
    std::error_code error;
    std::filesystem::remove(m_photoStore.absoluteImagePath(photo), error);
  }
}

std::vector<ClothingRepository::StoredItem>
ClothingRepository::listStored() const {
  std::vector<StoredItem> items;

  const bool success = m_database.query(
      "SELECT i.clothing_item_id, c.category_name, i.item_name, "
      "i.image_path, i.sub_category, i.primary_color, i.formality "
      "FROM clothing_items i "
      "JOIN clothing_categories c ON c.category_id = i.category_id "
      "ORDER BY i.clothing_item_id",
      {}, [&](const Database::Row &row) {
        const qint64 id = std::stoll(valueOrEmpty(row, 0));
        const auto category = fromText(kCategories, valueOrEmpty(row, 1));
        const auto sub = fromText(kSubCategories, valueOrEmpty(row, 4));
        const auto colour = fromText(kColours, valueOrEmpty(row, 5));
        const auto formality = fromText(kFormalities, valueOrEmpty(row, 6));
        if (!category || !sub || !colour || !formality) {
          qWarning() << "Skipping item" << id << "with unknown labels";
          return;
        }

        try {
          StoredItem stored;
          stored.id = id;
          stored.item = std::make_unique<ClothingItem>(
              valueOrEmpty(row, 2), *category, *sub, *colour, *formality,
              m_photoStore.absoluteImagePath(valueOrEmpty(row, 3)));
          stored.ml = QJsonObject{
              {"clothing_item_id", id},
              {"category_name",
               QString::fromStdString(valueOrEmpty(row, 1)).toLower()},
              {"sub_category", QString::fromStdString(valueOrEmpty(row, 4))},
              {"primary_color", QString::fromStdString(valueOrEmpty(row, 5))},
              {"formality", QString::fromStdString(valueOrEmpty(row, 6))}};
          items.push_back(std::move(stored));
        } catch (const std::exception &e) {
          qWarning() << "Skipping item" << id << e.what();
        }
      });
  if (!success)
    throw DatabaseError("Cannot load clothing items");

  return items;
}
