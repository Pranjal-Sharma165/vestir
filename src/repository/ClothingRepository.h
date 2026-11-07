#ifndef CLOTHINGREPOSITORY_H
#define CLOTHINGREPOSITORY_H

#include "ClothingItem.h"

#include <QJsonObject>
#include <QtTypes>
#include <memory>
#include <optional>
#include <vector>

/*!
 * \brief Saves and loads clothing items in the local database.
 *
 * Database::initialize() must have succeeded first. Photos are stored by
 * PhotoStore; the database only keeps their relative path.
 */
class ClothingRepository {
public:
  /*! A saved item with its database ID and the ML request form of its labels. */
  struct StoredItem {
    qint64 id = 0;
    std::unique_ptr<ClothingItem> item;
    // {clothing_item_id, category_name, sub_category, primary_color,
    // formality}: what python/recommend_cli.py expects in "items".
    QJsonObject ml;
  };

  /*! The labels of one item, as the app's enums. */
  struct Labels {
    ClothingCategory::Value category;
    ClothingCategory::SubCategory subCategory;
    ClothingColour::Value colour;
    OutfitFormality::Value formality;
  };

  /*!
   * Converts the ML classifier's answer (category_name, sub_category,
   * primary_color, formality) into enums. Empty if any label is unknown.
   */
  static std::optional<Labels> labelsFromMl(const QJsonObject &ml);

  /*!
   * Saves \a item. Its image path must point at a temporary photo (from the
   * camera or gallery): the photo is copied into wardrobe/ and the temporary
   * file is removed. Returns the new item ID, or 0 on failure (nothing is
   * saved and the temporary photo is left untouched).
   */
  static qint64 add(const ClothingItem &item);

  /*!
   * Loads every saved item, oldest first. Image paths are absolute. Rows with
   * labels the app does not know are skipped with a warning.
   */
  static std::vector<std::unique_ptr<ClothingItem>> list();

  /*! Same as list(), but also returns each item's ID and ML labels. */
  static std::vector<StoredItem> listStored();
};

#endif // CLOTHINGREPOSITORY_H
