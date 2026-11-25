#include "ClothingRepository.h"
#include "Database.h"
#include "EnumStringMappings.h"
#include "PhotoStore.h"

#include <QDebug>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <algorithm>
#include <array>
#include <optional>

namespace {

using Category = ClothingCategory::Value;
using SubCategory = ClothingCategory::SubCategory;
using Colour = ClothingColour::Value;
using Formality = OutfitFormality::Value;

template <typename E, size_t N>
QString toText(const std::array<Label<E>, N> &table, E value) {
  for (const auto &l : table)
    if (l.value == value)
      return QString::fromLatin1(l.text);
  return {};
}

template <typename E, size_t N>
std::optional<E> fromText(const std::array<Label<E>, N> &table,
                          const QString &text) {
  for (const auto &l : table)
    if (text == QLatin1String(l.text))
      return l.value;
  return std::nullopt;
}

} // namespace

std::optional<ClothingRepository::Labels>
ClothingRepository::labelsFromMl(const QJsonObject &ml) {
  // The ML writes the category in lowercase; the database table has "Top".
  QString category = ml.value("category_name").toString();
  if (!category.isEmpty())
    category[0] = category[0].toUpper();

  const auto cat = fromText(kCategories, category);
  const auto sub =
      fromText(kSubCategories, ml.value("sub_category").toString());
  const auto colour = fromText(kColours, ml.value("primary_color").toString());
  const auto formality =
      fromText(kFormalities, ml.value("formality").toString());
  if (!cat || !sub || !colour || !formality)
    return std::nullopt;
  return Labels{*cat, *sub, *colour, *formality};
}

qint64 ClothingRepository::add(const ClothingItem &item) {
  const QString tempPath = item.qImageUrl().toLocalFile();
  const QString storedPath = PhotoStore::addPhoto(tempPath, false);
  if (storedPath.isEmpty())
    return 0;

  QSqlQuery q(Database::connection());
  q.prepare("INSERT INTO clothing_items "
            "(category_id, item_name, image_path, sub_category, primary_color, "
            "formality) VALUES ("
            "(SELECT category_id FROM clothing_categories "
            "WHERE category_name = :category), "
            ":name, :path, :sub, :colour, :formality)");
  q.bindValue(":category", toText(kCategories, item.category()));
  q.bindValue(":name", QString::fromStdString(item.name()));
  q.bindValue(":path", storedPath);
  q.bindValue(":sub", toText(kSubCategories, item.subCategory()));
  q.bindValue(":colour", toText(kColours, item.colour()));
  q.bindValue(":formality", toText(kFormalities, item.formality()));

  if (!q.exec()) {
    qWarning() << "Cannot save clothing item:" << q.lastError().text();
    QFile::remove(Database::absoluteImagePath(storedPath));
    return 0;
  }
  PhotoStore::discardPhoto(tempPath);
  return q.lastInsertId().toLongLong();
}

void ClothingRepository::update(qint64 id, const Labels &labels) {
  if (!ClothingCategory::subcategoryIsInCategory(labels.subCategory,
                                                 labels.category))
    throw InvalidInputError(
        "That sub-category does not belong to the category");

  QSqlQuery q(Database::connection());
  q.prepare("UPDATE clothing_items SET "
            "category_id = (SELECT category_id FROM clothing_categories "
            "WHERE category_name = :category), "
            "sub_category = :sub, primary_color = :colour, "
            "formality = :formality WHERE clothing_item_id = :id");
  q.bindValue(":category", toText(kCategories, labels.category));
  q.bindValue(":sub", toText(kSubCategories, labels.subCategory));
  q.bindValue(":colour", toText(kColours, labels.colour));
  q.bindValue(":formality", toText(kFormalities, labels.formality));
  q.bindValue(":id", id);
  if (!q.exec())
    throw DatabaseError("Cannot save the changes");
  if (q.numRowsAffected() == 0)
    throw NotFoundError("This clothing item no longer exists");
}

void ClothingRepository::remove(qint64 id) {
  QSqlQuery find(Database::connection());
  find.prepare("SELECT image_path FROM clothing_items "
               "WHERE clothing_item_id = :id");
  find.bindValue(":id", id);
  if (!find.exec())
    throw DatabaseError("Cannot delete the item");
  if (!find.next())
    throw NotFoundError("This clothing item no longer exists");
  const QString photo = find.value(0).toString();
  find.finish();

  QSqlQuery q(Database::connection());
  q.prepare("DELETE FROM clothing_items WHERE clothing_item_id = :id");
  q.bindValue(":id", id);
  if (!q.exec())
    throw DatabaseError("Cannot delete the item");
  if (!photo.isEmpty())
    QFile::remove(Database::absoluteImagePath(photo));
}

std::vector<ClothingRepository::StoredItem> ClothingRepository::listStored() {
  std::vector<StoredItem> items;

  QSqlQuery q(Database::connection());
  if (!q.exec("SELECT i.clothing_item_id, c.category_name, i.item_name, "
              "i.image_path, i.sub_category, i.primary_color, i.formality "
              "FROM clothing_items i "
              "JOIN clothing_categories c ON c.category_id = i.category_id "
              "ORDER BY i.clothing_item_id")) {
    qWarning() << "Cannot load clothing items:" << q.lastError().text();
    return items;
  }

  while (q.next()) {
    const auto category = fromText(kCategories, q.value(1).toString());
    const auto sub = fromText(kSubCategories, q.value(4).toString());
    const auto colour = fromText(kColours, q.value(5).toString());
    const auto formality = fromText(kFormalities, q.value(6).toString());
    if (!category || !sub || !colour || !formality) {
      qWarning() << "Skipping item" << q.value(0).toLongLong()
                 << "with unknown labels";
      continue;
    }

    try {
      StoredItem stored;
      stored.id = q.value(0).toLongLong();
      stored.item = std::make_unique<ClothingItem>(
          q.value(2).toString().toStdString(), *category, *sub, *colour,
          *formality,
          Database::absoluteImagePath(q.value(3).toString()).toStdString());
      stored.ml =
          QJsonObject{{"clothing_item_id", stored.id},
                      {"category_name", q.value(1).toString().toLower()},
                      {"sub_category", q.value(4).toString()},
                      {"primary_color", q.value(5).toString()},
                      {"formality", q.value(6).toString()}};
      items.push_back(std::move(stored));
    } catch (const std::exception &e) {
      qWarning() << "Skipping item" << q.value(0).toLongLong() << e.what();
    }
  }
  return items;
}
