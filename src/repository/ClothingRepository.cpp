#include "ClothingRepository.h"
#include "Database.h"
#include "PhotoStore.h"

#include <QDebug>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <algorithm>
#include <array>
#include <optional>

namespace {

template <typename E> struct Label {
  E value;
  const char *text;
};

using Category = ClothingCategory::Value;
using SubCategory = ClothingCategory::SubCategory;
using Colour = ClothingColour::Value;
using Formality = OutfitFormality::Value;

// Database labels. They must match the CHECK constraints in schema.sql.
constexpr std::array kCategories{
    Label<Category>{Category::Top, "Top"},
    Label<Category>{Category::Bottom, "Bottom"},
    Label<Category>{Category::Outerwear, "Outerwear"},
    Label<Category>{Category::Dress, "Dress"},
};

constexpr std::array kSubCategories{
    Label<SubCategory>{SubCategory::TShirt, "t-shirt"},
    Label<SubCategory>{SubCategory::Blouse, "blouse"},
    Label<SubCategory>{SubCategory::Sweater, "sweater"},
    Label<SubCategory>{SubCategory::Hoodie, "hoodie"},
    Label<SubCategory>{SubCategory::Shirt, "shirt"},
    Label<SubCategory>{SubCategory::TankTop, "tank top"},
    Label<SubCategory>{SubCategory::Jeans, "jeans"},
    Label<SubCategory>{SubCategory::Trousers, "trousers"},
    Label<SubCategory>{SubCategory::Shorts, "shorts"},
    Label<SubCategory>{SubCategory::Skirt, "skirt"},
    Label<SubCategory>{SubCategory::Dress, "dress"},
    Label<SubCategory>{SubCategory::Jumpsuit, "jumpsuit"},
    Label<SubCategory>{SubCategory::Romper, "romper"},
    Label<SubCategory>{SubCategory::Jacket, "jacket"},
    Label<SubCategory>{SubCategory::Coat, "coat"},
    Label<SubCategory>{SubCategory::Blazer, "blazer"},
    Label<SubCategory>{SubCategory::Parka, "parka"},
    Label<SubCategory>{SubCategory::Cardigan, "cardigan"},
    Label<SubCategory>{SubCategory::Vest, "vest"},
};

constexpr std::array kColours{
    Label<Colour>{Colour::Black, "black"},
    Label<Colour>{Colour::White, "white"},
    Label<Colour>{Colour::Charcoal, "charcoal"},
    Label<Colour>{Colour::Brown, "brown"},
    Label<Colour>{Colour::Beige, "beige"},
    Label<Colour>{Colour::Khaki, "khaki"},
    Label<Colour>{Colour::Cream, "cream"},
    Label<Colour>{Colour::NavyBlue, "navy blue"},
    Label<Colour>{Colour::Blue, "blue"},
    Label<Colour>{Colour::LightBlue, "light blue"},
    Label<Colour>{Colour::Green, "green"},
    Label<Colour>{Colour::OliveGreen, "olive green"},
    Label<Colour>{Colour::Red, "red"},
    Label<Colour>{Colour::Pink, "pink"},
    Label<Colour>{Colour::Purple, "purple"},
    Label<Colour>{Colour::Yellow, "yellow"},
    Label<Colour>{Colour::Orange, "orange"},
    Label<Colour>{Colour::Burgundy, "burgundy"},
};

constexpr std::array kFormalities{
    Label<Formality>{Formality::Casual, "casual"},
    Label<Formality>{Formality::SmartCasual, "smart-casual"},
    Label<Formality>{Formality::Formal, "formal"},
    Label<Formality>{Formality::Sporty, "sporty"},
};

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

std::vector<std::unique_ptr<ClothingItem>> ClothingRepository::list() {
  std::vector<std::unique_ptr<ClothingItem>> items;

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
      items.push_back(std::make_unique<ClothingItem>(
          q.value(2).toString().toStdString(), *category, *sub, *colour,
          *formality,
          Database::absoluteImagePath(q.value(3).toString()).toStdString()));
    } catch (const std::exception &e) {
      qWarning() << "Skipping item" << q.value(0).toLongLong() << e.what();
    }
  }
  return items;
}
