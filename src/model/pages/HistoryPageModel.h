#ifndef HISTORYPAGEMODEL_H
#define HISTORYPAGEMODEL_H

#include "../../backend/ClothingRepository.h"
#include "../../backend/Errors.h"
#include "../../backend/HistoryRepository.h"
#include <QDateTime>
#include <QDebug>
#include <QQmlEngine>
#include <QTimeZone>
#include <QVariantList>
#include <QVariantMap>
#include <unordered_map>

/*!
 * \brief Data source for the History page: the outfits the user chose, newest
 * first. It is a log only; nothing is sent back to the ML.
 */
class HistoryPageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QVariantList entries READ entries NOTIFY entriesChanged)

signals:
  void entriesChanged();

public:
  explicit HistoryPageModel(ClothingRepository &closet,
                            HistoryRepository &history,
                            QObject *parent = nullptr)
      : QObject(parent), m_closet(closet), m_history(history) {
    reload();
  }

  QVariantList entries() const { return m_entries; }

  Q_INVOKABLE void reload() {
    QVariantList list;
    try {
      const auto closet = m_closet.listStored();
      std::unordered_map<qint64, const ClothingItem *> byId;
      for (const auto &s : closet)
        byId[s.id] = s.item.get();

      for (const HistoryEntry &h : m_history.chosenOutfits()) {
        QVariantList pieces;
        for (qint64 id : h.itemIds)
          if (auto it = byId.find(id); it != byId.end())
            pieces.append(QVariantMap{{"name", it->second->qName()},
                                      {"imageUrl", it->second->qImageUrl()}});
        if (pieces.isEmpty())
          continue; // every item of this outfit was deleted
        QDateTime when = QDateTime::fromString(
            QString::fromStdString(h.chosenAtUtc), "yyyy-MM-dd HH:mm:ss");
        when.setTimeZone(QTimeZone::UTC);
        const QString eventName = QString::fromStdString(h.eventName);
        list.append(QVariantMap{
            {"title", eventName.isEmpty() ? tr("Everyday outfit") : eventName},
            {"when", when.toLocalTime()},
            {"pieces", pieces}});
      }
    } catch (const VestirError &e) {
      qWarning() << "History:" << e.what();
    }
    m_entries = list;
    emit entriesChanged();
  }

private:
  ClothingRepository &m_closet;
  HistoryRepository &m_history;
  QVariantList m_entries;
};

#endif // HISTORYPAGEMODEL_H
