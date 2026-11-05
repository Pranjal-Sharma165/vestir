#ifndef CLOTHINGREPOSITORY_H
#define CLOTHINGREPOSITORY_H

#include <QList>
#include <QString>
#include <optional>

/*!
 * \brief One row of the clothing_items table, with the category name joined in.
 *
 * Strings use the exact values the ML classifier returns (lowercase, e.g.
 * "t-shirt", "navy blue", "smart-casual"). Category is the DB name:
 * "Top", "Bottom", "Dress" or "Outerwear". Empty strings / negative
 * confidences mean "not set".
 */
struct ClothingRecord {
  int id = 0;
  QString category;
  QString itemName;
  QString imagePath;
  QString subCategory;
  QString primaryColor;
  QString formality;
  QString season;
  double categoryConfidence = -1;
  double subCategoryConfidence = -1;
  double colorConfidence = -1;
  double formalityConfidence = -1;
};

/*!
 * \brief CRUD access to clothing_items. No UI or Qt object dependencies.
 */
class ClothingRepository {
public:
  QList<ClothingRecord> listAll() const;
  QList<ClothingRecord> listByCategory(const QString &category) const;
  std::optional<ClothingRecord> get(int id) const;

  /*! Inserts the item and returns its new id, or -1 on failure. */
  int add(const ClothingRecord &item) const;

  /*! Updates every editable field of the item with item.id. */
  bool update(const ClothingRecord &item) const;

  bool remove(int id) const;
};

#endif // CLOTHINGREPOSITORY_H
