#ifndef CLOSETPAGEMODEL_H
#define CLOSETPAGEMODEL_H

#include "ClothingCategory.h"
#include "ClothingItem.h"
#include "../../backend/ClothingRepository.h"
#include "../../backend/PhotoStore.h"

#include <QQmlEngine>

/*!
 * \brief The Closet class is the data source for the closet page. It provides
 * read-write access to clothing items in the closet.
 *
 * Intended for use by the frontend to read clothing items and write changes
 * that the user makes to the list of clothing items as a whole. It allows:
 * - Reading the list of clothing items (sorting & filtering should be done by
 * the frontend, given the few clothing items most users will have)
 * - Adding new clothing items
 * - Removing clothing items
 * - Receiving signals when the contents of the closet change
 *
 * Note: Changes to clothing items themselves are not signalled here. Those
 * changes are signalled within the respecitive ClothingItem instance.
 */
class Closet : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QList<ClothingItem *> items READ items NOTIFY itemsListChanged)

signals:
  /*!
   * Signal that the number of clothes in the closet have changed.
   */
  void itemsListChanged();

public:
  explicit Closet(QObject *parent = nullptr) : QObject(parent) {
    reload();
  }

  /*! Saves the item and its photo to the database; false on failure. */
  Q_INVOKABLE bool addItem(ClothingItem *item) {
    if (!item || ClothingRepository::add(*item) == 0)
      return false;
    reload();
    return true;
  }

  /*!
   * Changes the labels of \a item (fixing a classifier mistake). Returns an
   * empty string on success, otherwise a message to show the user. \a item is
   * replaced by a fresh object: leave the page that showed it afterwards.
   */
  Q_INVOKABLE QString updateItem(ClothingItem *item,
                                 ClothingCategory::Value category,
                                 ClothingCategory::SubCategory subCategory,
                                 ClothingColour::Value colour,
                                 OutfitFormality::Value formality) {
    const qsizetype index = m_items.indexOf(item);
    if (index < 0)
      return "This clothing item no longer exists";
    try {
      ClothingRepository::update(m_ids.at(index),
                                 {category, subCategory, colour, formality});
    } catch (const VestirError &e) {
      return e.message();
    }
    reload();
    return {};
  }

  /*!
   * Deletes \a item and its photo. Returns an empty string on success,
   * otherwise a message to show the user. Leave the page that showed the item
   * afterwards.
   */
  Q_INVOKABLE QString removeItem(ClothingItem *item) {
    const qsizetype index = m_items.indexOf(item);
    if (index < 0)
      return "This clothing item no longer exists";
    try {
      ClothingRepository::remove(m_ids.at(index));
    } catch (const VestirError &e) {
      return e.message();
    }
    reload();
    return {};
  }

  /*! Deletes a temporary photo the user did not keep. */
  Q_INVOKABLE void discardPhoto(const QUrl &photo) {
    PhotoStore::discardPhoto(photo.toLocalFile());
  }

  QList<ClothingItem *> items() const { return m_items; }

  Q_INVOKABLE int count() const { return m_items.count(); }

  Q_INVOKABLE int count(ClothingCategory::Value category) const {
    return std::count_if(m_items.cbegin(), m_items.cend(),
                         [category](const ClothingItem *item) -> bool {
                           return item->category() == category;
                         });
  }

private:
  void reload() {
    for (ClothingItem *old : std::as_const(m_items))
      old->deleteLater();
    m_items.clear();
    m_ids.clear();
    for (auto &stored : ClothingRepository::listStored()) {
      stored.item->setParent(this);
      m_items.append(stored.item.release());
      m_ids.append(stored.id);
    }
    emit itemsListChanged();
  }

  QList<ClothingItem *> m_items;
  QList<qint64> m_ids; // database ID of each item in m_items
};

#endif // CLOSETPAGEMODEL_H
