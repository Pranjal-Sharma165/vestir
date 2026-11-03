#ifndef CLOSET_H
#define CLOSET_H

#include "ClothingItem.h"
#include <QQmlEngine>

/*!
 * \brief The Closet class provides read-write access to clothing
 * items in the closet.
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
  explicit Closet(QObject *parent = nullptr) {
    // TODO replace mock data with real call to database
    for (int i = 1; i < 4; i++) {
      m_items.append(new ClothingItem(std::format("Shirt {}", i),
                                      ClothingCategory::Top, this));
      m_items.append(new ClothingItem(std::format("Pants {}", i),
                                      ClothingCategory::Bottom, this));
      m_items.append(new ClothingItem(std::format("Coat {}", i),
                                      ClothingCategory::Outerwear, this));
      m_items.append(new ClothingItem(std::format("Shoes {}", i),
                                      ClothingCategory::Shoe, this));
    }
  }

  QList<ClothingItem *> items() const { return m_items; }

  void refreshItems();

private:
  QList<ClothingItem *> m_items;
};

#endif // CLOSET_H
