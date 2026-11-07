#ifndef CLOTHINGREPOSITORY_H
#define CLOTHINGREPOSITORY_H

#include "ClothingItem.h"

#include <QtTypes>
#include <memory>
#include <vector>

/*!
 * \brief Saves and loads clothing items in the local database.
 *
 * Database::initialize() must have succeeded first. Photos are stored by
 * PhotoStore; the database only keeps their relative path.
 */
class ClothingRepository {
public:
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
};

#endif // CLOTHINGREPOSITORY_H
