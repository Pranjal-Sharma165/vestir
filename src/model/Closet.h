#ifndef CLOSET_H
#define CLOSET_H

#include "ClothingCategory.h"
#include "ClothingItem.h"

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
    // TODO replace mock data with real call to database
    for (int i = 1; i < 10; i++) {
      if (i % 2) {
        m_items.append(new ClothingItem(std::format("Shirt {}", i),
                                        ClothingCategory::Value::Top, this));
      }
      if (i % 3) {
        m_items.append(new ClothingItem(std::format("Pants {}", i),
                                        ClothingCategory::Value::Bottom, this));
      }
      if (i % 4) {
        m_items.append(new ClothingItem(std::format("Coat {}", i),
                                        ClothingCategory::Value::Outerwear,
                                        this));
      }
      if (i % 5) {
        m_items.append(new ClothingItem(std::format("Shoes {}", i),
                                        ClothingCategory::Value::Shoe, this));
      }
    }
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
  QList<ClothingItem *> m_items;
};

#endif // CLOSET_H
