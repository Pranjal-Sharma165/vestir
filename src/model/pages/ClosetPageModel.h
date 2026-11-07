#ifndef CLOSETPAGEMODEL_H
#define CLOSETPAGEMODEL_H

#include "ClothingCategory.h"
#include "ClothingItem.h"
#include "../../repository/ClothingRepository.h"
#include "../../repository/PhotoStore.h"

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
    for (auto &item : ClothingRepository::list()) {
      item->setParent(this);
      m_items.append(item.release());
    }
    emit itemsListChanged();
  }

  QList<ClothingItem *> m_items;
};

#endif // CLOSETPAGEMODEL_H
