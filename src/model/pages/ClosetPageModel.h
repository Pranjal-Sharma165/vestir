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
 */
class Closet : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QList<ClothingItem *> items READ items NOTIFY itemsListChanged)

signals:
  // Signal that the number of clothes in the closet have changed.
  void itemsListChanged();

public:
  explicit Closet(ClothingRepository &repository, PhotoStore &photoStore,
                  QObject *parent = nullptr)
      : QObject(parent), m_repository(repository), m_photoStore(photoStore) {
    reload();
  }

  Q_INVOKABLE bool addItem(ClothingItem *item) {
    if (!item || m_repository.add(*item) == 0)
      return false;
    reload();
    return true;
  }

  Q_INVOKABLE QString updateItem(ClothingItem *item,
                                 ClothingCategory::Value category,
                                 ClothingCategory::SubCategory subCategory,
                                 ClothingColour::Value colour,
                                 OutfitFormality::Value formality) {
    const qsizetype index = m_items.indexOf(item);
    if (index < 0)
      return "This clothing item no longer exists";
    try {
      m_repository.update(m_ids.at(index),
                          {category, subCategory, colour, formality});
    } catch (const VestirError &e) {
      return QString::fromUtf8(e.what());
    }
    reload();
    return {};
  }

  Q_INVOKABLE QString removeItem(ClothingItem *item) {
    const qsizetype index = m_items.indexOf(item);
    if (index < 0)
      return "This clothing item no longer exists";
    try {
      m_repository.remove(m_ids.at(index));
    } catch (const VestirError &e) {
      return QString::fromUtf8(e.what());
    }
    reload();
    return {};
  }

  Q_INVOKABLE void discardPhoto(const QUrl &photo) {
    m_photoStore.discardPhoto(photo.toLocalFile().toStdString());
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
    for (auto &stored : m_repository.listStored()) {
      stored.item->setParent(this);
      m_items.append(stored.item.release());
      m_ids.append(stored.id);
    }
    emit itemsListChanged();
  }

  QList<ClothingItem *> m_items;
  QList<qint64> m_ids; // database ID of each item in m_items
  ClothingRepository &m_repository;
  PhotoStore &m_photoStore;
};

#endif // CLOSETPAGEMODEL_H
