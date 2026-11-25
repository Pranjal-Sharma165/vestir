#include "ClothingRepository.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <string_view>

namespace {

constexpr std::array<std::string_view, 4> kCategories{
    "Top", "Bottom", "Outerwear", "Dress"};

constexpr std::array<std::string_view, 20> kSubCategories{
    "t-shirt", "blouse", "sweater", "hoodie", "shirt", "tank top",
    "jeans", "trousers", "shorts", "skirt", "leggings", "dress",
    "jumpsuit", "romper", "jacket", "coat", "blazer", "parka",
    "cardigan", "vest"};

constexpr std::array<std::string_view, 19> kColours{
    "black", "white", "charcoal", "gray", "brown", "beige", "khaki",
    "cream", "navy blue", "blue", "light blue", "green", "olive green",
    "red", "pink", "purple", "yellow", "orange", "burgundy"};

constexpr std::array<std::string_view, 4> kFormalities{
    "casual", "smart-casual", "formal", "sporty"};

bool contains(const auto &values, const std::string &value) {
  return std::find(values.begin(), values.end(), value) != values.end();
}

std::string normalizedCategory(std::string value) {
  if (!value.empty())
    value.front() = static_cast<char>(
        std::toupper(static_cast<unsigned char>(value.front())));
  return value;
}

bool subCategoryMatches(const std::string &category,
                        const std::string &subCategory) {
  if (category == "Top")
    return subCategory == "t-shirt" || subCategory == "blouse" ||
           subCategory == "sweater" || subCategory == "hoodie" ||
           subCategory == "shirt" || subCategory == "tank top";
  if (category == "Bottom")
    return subCategory == "jeans" || subCategory == "trousers" ||
           subCategory == "shorts" || subCategory == "skirt" ||
           subCategory == "leggings";
  if (category == "Dress")
    return subCategory == "dress" || subCategory == "jumpsuit" ||
           subCategory == "romper";
  if (category == "Outerwear")
    return subCategory == "jacket" || subCategory == "coat" ||
           subCategory == "blazer" || subCategory == "parka" ||
           subCategory == "cardigan" || subCategory == "vest";
  return false;
}

bool validLabels(const ClothingRepository::Labels &labels) {
  return contains(kCategories, labels.category) &&
         contains(kSubCategories, labels.subCategory) &&
         contains(kColours, labels.colour) &&
         contains(kFormalities, labels.formality) &&
         subCategoryMatches(labels.category, labels.subCategory);
}

std::string valueOrEmpty(const Database::Row &row, std::size_t index) {
  if (index >= row.size() || !row[index])
    return {};
  return *row[index];
}

} // namespace

ClothingRepository::ClothingRepository(Database &database,
                                       PhotoStore &photoStore)
    : m_database(database), m_photoStore(photoStore) {}

std::optional<ClothingRepository::Labels> ClothingRepository::labelsFromMl(const MlLabels &ml) {
  Labels labels{normalizedCategory(ml.categoryName), ml.subCategory,
                ml.primaryColor, ml.formality};
  if (!validLabels(labels))
    return std::nullopt;
  return labels;
}

std::int64_t ClothingRepository::add(const Item &item) {
  if (!validLabels(item.labels))
    throw InvalidInputError("The clothing item has invalid labels");

  const std::filesystem::path storedPath =
      m_photoStore.addPhoto(item.imagePath, false);
  if (storedPath.empty())
    return 0;

  const bool saved = m_database.execute(
      "INSERT INTO clothing_items "
      "(category_id, item_name, image_path, sub_category, primary_color, "
      "formality) VALUES ("
      "(SELECT category_id FROM clothing_categories WHERE category_name = ?), "
      "?, ?, ?, ?, ?)",
      {item.labels.category, item.name, storedPath.string(),
       item.labels.subCategory, item.labels.colour, item.labels.formality});

  if (!saved) {
    std::error_code error;
    std::filesystem::remove(m_photoStore.absoluteImagePath(storedPath), error);
    return 0;
  }
  m_photoStore.discardPhoto(item.imagePath);
  return m_database.lastInsertId();
}

void ClothingRepository::update(std::int64_t id, const Labels &labels) {
  if (!validLabels(labels))
    throw InvalidInputError("The clothing item has invalid labels");

  const bool saved = m_database.execute(
      "UPDATE clothing_items SET "
      "category_id = (SELECT category_id FROM clothing_categories "
      "WHERE category_name = ?), "
      "sub_category = ?, primary_color = ?, formality = ? "
      "WHERE clothing_item_id = ?",
      {labels.category, labels.subCategory, labels.colour, labels.formality,
       std::to_string(id)});
  if (!saved)
    throw DatabaseError("Cannot save the changes");
  if (m_database.changes() == 0)
    throw NotFoundError("This clothing item no longer exists");
}

void ClothingRepository::remove(std::int64_t id) {
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
        Labels labels{valueOrEmpty(row, 1), valueOrEmpty(row, 4),
                      valueOrEmpty(row, 5), valueOrEmpty(row, 6)};
        if (!validLabels(labels))
          return;

        StoredItem stored;
        stored.id = std::stoll(valueOrEmpty(row, 0));
        stored.item.id = stored.id;
        stored.item.name = valueOrEmpty(row, 2);
        stored.item.labels = std::move(labels);
        stored.item.imagePath =
            m_photoStore.absoluteImagePath(valueOrEmpty(row, 3));
        items.push_back(std::move(stored));
      });
  if (!success)
    throw DatabaseError("Cannot load clothing items");

  return items;
}
