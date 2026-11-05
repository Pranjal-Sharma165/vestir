#include "ClothingRepository.h"
#include "Database.h"

#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {

const char *kSelect =
    "SELECT i.clothing_item_id, c.category_name, i.item_name, i.image_path, "
    "i.sub_category, i.primary_color, i.formality, i.season, "
    "i.category_confidence, i.sub_category_confidence, "
    "i.color_confidence, i.formality_confidence "
    "FROM clothing_items i "
    "LEFT JOIN clothing_categories c ON c.category_id = i.category_id ";

QVariant textOrNull(const QString &s) {
  return s.isEmpty() ? QVariant(QMetaType::fromType<QString>()) : QVariant(s);
}

QVariant realOrNull(double v) {
  return v < 0 ? QVariant(QMetaType::fromType<double>()) : QVariant(v);
}

double realOrUnset(const QVariant &v) { return v.isNull() ? -1 : v.toDouble(); }

ClothingRecord fromQuery(const QSqlQuery &q) {
  ClothingRecord r;
  r.id = q.value(0).toInt();
  r.category = q.value(1).toString();
  r.itemName = q.value(2).toString();
  r.imagePath = q.value(3).toString();
  r.subCategory = q.value(4).toString();
  r.primaryColor = q.value(5).toString();
  r.formality = q.value(6).toString();
  r.season = q.value(7).toString();
  r.categoryConfidence = realOrUnset(q.value(8));
  r.subCategoryConfidence = realOrUnset(q.value(9));
  r.colorConfidence = realOrUnset(q.value(10));
  r.formalityConfidence = realOrUnset(q.value(11));
  return r;
}

QList<ClothingRecord> readAll(QSqlQuery &q) {
  QList<ClothingRecord> out;
  if (!q.exec()) {
    qWarning() << "Query failed:" << q.lastError().text();
    return out;
  }
  while (q.next())
    out.append(fromQuery(q));
  return out;
}

void bindFields(QSqlQuery &q, const ClothingRecord &item) {
  q.addBindValue(item.category);
  q.addBindValue(textOrNull(item.itemName));
  q.addBindValue(item.imagePath);
  q.addBindValue(textOrNull(item.subCategory));
  q.addBindValue(textOrNull(item.primaryColor));
  q.addBindValue(textOrNull(item.formality));
  q.addBindValue(textOrNull(item.season));
  q.addBindValue(realOrNull(item.categoryConfidence));
  q.addBindValue(realOrNull(item.subCategoryConfidence));
  q.addBindValue(realOrNull(item.colorConfidence));
  q.addBindValue(realOrNull(item.formalityConfidence));
}

} // namespace

QList<ClothingRecord> ClothingRepository::listAll() const {
  QSqlQuery q(Database::connection());
  q.prepare(QString(kSelect) + "ORDER BY i.clothing_item_id");
  return readAll(q);
}

QList<ClothingRecord>
ClothingRepository::listByCategory(const QString &category) const {
  QSqlQuery q(Database::connection());
  q.prepare(QString(kSelect) +
            "WHERE c.category_name = ? ORDER BY i.clothing_item_id");
  q.addBindValue(category);
  return readAll(q);
}

std::optional<ClothingRecord> ClothingRepository::get(int id) const {
  QSqlQuery q(Database::connection());
  q.prepare(QString(kSelect) + "WHERE i.clothing_item_id = ?");
  q.addBindValue(id);
  const auto rows = readAll(q);
  if (rows.isEmpty())
    return std::nullopt;
  return rows.first();
}

int ClothingRepository::add(const ClothingRecord &item) const {
  QSqlQuery q(Database::connection());
  q.prepare("INSERT INTO clothing_items (category_id, item_name, image_path, "
            "sub_category, primary_color, formality, season, "
            "category_confidence, sub_category_confidence, color_confidence, "
            "formality_confidence) "
            "VALUES ((SELECT category_id FROM clothing_categories "
            "WHERE category_name = ?), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
  bindFields(q, item);
  if (!q.exec()) {
    qWarning() << "Insert failed:" << q.lastError().text();
    return -1;
  }
  return q.lastInsertId().toInt();
}

bool ClothingRepository::update(const ClothingRecord &item) const {
  QSqlQuery q(Database::connection());
  q.prepare("UPDATE clothing_items SET "
            "category_id = (SELECT category_id FROM clothing_categories "
            "WHERE category_name = ?), "
            "item_name = ?, image_path = ?, sub_category = ?, "
            "primary_color = ?, formality = ?, season = ?, "
            "category_confidence = ?, sub_category_confidence = ?, "
            "color_confidence = ?, formality_confidence = ? "
            "WHERE clothing_item_id = ?");
  bindFields(q, item);
  q.addBindValue(item.id);
  if (!q.exec()) {
    qWarning() << "Update failed:" << q.lastError().text();
    return false;
  }
  return q.numRowsAffected() > 0;
}

bool ClothingRepository::remove(int id) const {
  QSqlQuery q(Database::connection());
  q.prepare("DELETE FROM clothing_items WHERE clothing_item_id = ?");
  q.addBindValue(id);
  if (!q.exec()) {
    qWarning() << "Delete failed:" << q.lastError().text();
    return false;
  }
  return q.numRowsAffected() > 0;
}
