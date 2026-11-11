#ifndef HISTORYPAGEMODEL_H
#define HISTORYPAGEMODEL_H

#include "../../repository/ClothingRepository.h"
#include "../../repository/Errors.h"
#include "../../repository/HistoryRepository.h"
#include <QDateTime>
#include <QDebug>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>
#include <unordered_map>

/*!
 * \brief Data source for the History page: the outfits the user chose, newest
 * first. It is a log only; nothing is sent back to the ML.
 *
 * entries is a list of {title, when, pieces}, where title is the event name
 * (or "Everyday outfit"), when is the date and pieces is a list of
 * {name, imageUrl}. Call reload() when the page is shown.
 */
class HistoryPageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)

signals:
  void entriesChanged();

public:
  explicit HistoryPageModel(QObject *parent = nullptr) : QObject(parent) {
    reload();
  }

  QVariantList entries() const { return m_entries; }

  Q_INVOKABLE void reload() {
    QVariantList list;
    try {
      const auto closet = ClothingRepository::listStored();
      std::unordered_map<qint64, const ClothingItem *> byId;
      for (const auto &s : closet)
        byId[s.id] = s.item.get();

      const auto history = HistoryRepository::chosenOutfits(
          QDateTime::fromSecsSinceEpoch(0), QDateTime::currentDateTime());
      for (const HistoryEntry &h : history) {
        QVariantList pieces;
        for (qint64 id : h.itemIds)
          if (auto it = byId.find(id); it != byId.end())
            pieces.append(QVariantMap{{"name", it->second->qName()},
                                      {"imageUrl", it->second->qImageUrl()}});
        if (pieces.isEmpty())
          continue; // every item of this outfit was deleted
        list.append(QVariantMap{
            {"title", h.eventName.isEmpty() ? tr("Everyday outfit")
                                            : h.eventName},
            {"when", h.chosenAt},
            {"pieces", pieces}});
      }
    } catch (const VestirError &e) {
      qWarning() << "History:" << e.message();
    }
    m_entries = list;
    emit entriesChanged();
  }

private:
  QVariantList m_entries;
};

#endif // HISTORYPAGEMODEL_H
