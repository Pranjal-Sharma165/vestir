#ifndef CLOSET_H
#define CLOSET_H

#include "../backend/ClothingRepository.h"
#include "ClothingCategory.h"
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
  explicit Closet(QObject *parent = nullptr) : QObject(parent) { reload(); }

  /*!
   * Reloads all items from the database. Requires Database::open() to have
   * succeeded; otherwise the closet is empty.
   */
  Q_INVOKABLE void reload() {
    for (ClothingItem *item : std::as_const(m_items))
      item->deleteLater();
    m_items.clear();

    for (const ClothingRecord &r : m_repository.listAll()) {
      const auto category = categoryFromDb(r.category);
      if (!category)
        continue;
      m_items.append(new ClothingItem(displayName(r).toStdString(), *category,
                                      this, r.id, r.imagePath));
    }
    emit itemsListChanged();
  }

  /*!
   * Adds an item to the database and the closet. sub_category, color and
   * formality must use the ML label values (empty if unknown).
   * \return the new item id, or -1 on failure.
   */
  Q_INVOKABLE int addItem(const QString &name,
                          ClothingCategory::Value category,
                          const QString &imagePath,
                          const QString &subCategory = QString(),
                          const QString &color = QString(),
                          const QString &formality = QString()) {
    ClothingRecord r;
    r.category = categoryToDb(category);
    r.itemName = name;
    r.imagePath = imagePath;
    r.subCategory = subCategory;
    r.primaryColor = color;
    r.formality = formality;

    const int id = m_repository.add(r);
    if (id < 0)
      return -1;
    m_items.append(new ClothingItem(displayName(r).toStdString(), category,
                                    this, id, imagePath));
    emit itemsListChanged();
    return id;
  }

  /*! Deletes the item with the given id from the database and the closet. */
  Q_INVOKABLE bool removeItem(int id) {
    if (!m_repository.remove(id))
      return false;
    for (int i = 0; i < m_items.size(); ++i) {
      if (m_items[i]->id() == id) {
        m_items.takeAt(i)->deleteLater();
        break;
      }
    }
    emit itemsListChanged();
    return true;
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
  // Items classified by the ML may have no user-given name; fall back to the
  // sub-category so the card is never blank.
  static QString displayName(const ClothingRecord &r) {
    if (!r.itemName.isEmpty())
      return r.itemName;
    QString name = r.primaryColor.isEmpty()
                       ? r.subCategory
                       : r.primaryColor + " " + r.subCategory;
    return name.trimmed().isEmpty() ? QStringLiteral("Clothing item") : name;
  }

  static std::optional<ClothingCategory::Value>
  categoryFromDb(const QString &name) {
    if (name == "Top")
      return ClothingCategory::Value::Top;
    if (name == "Bottom")
      return ClothingCategory::Value::Bottom;
    if (name == "Dress")
      return ClothingCategory::Value::Dress;
    if (name == "Outerwear")
      return ClothingCategory::Value::Outerwear;
    return std::nullopt;
  }

  static QString categoryToDb(ClothingCategory::Value category) {
    switch (category) {
    case ClothingCategory::Value::Top:
      return "Top";
    case ClothingCategory::Value::Bottom:
      return "Bottom";
    case ClothingCategory::Value::Dress:
      return "Dress";
    case ClothingCategory::Value::Outerwear:
      return "Outerwear";
    }
    return QString();
  }

  ClothingRepository m_repository;
  QList<ClothingItem *> m_items;
};

#endif // CLOSET_H
