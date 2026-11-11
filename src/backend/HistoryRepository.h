#ifndef HISTORYREPOSITORY_H
#define HISTORYREPOSITORY_H

#include "Errors.h"

#include <QDateTime>
#include <QString>
#include <QtTypes>
#include <vector>

/*! One outfit the user chose to wear. */
struct HistoryEntry {
  qint64 recommendationId = 0;
  QDateTime chosenAt; // local time
  QString eventName;  // empty if the outfit was not for an event
  QString eventType;  // school/work/party/hangout/gym, empty if unknown
  // Items still in the closet. Deleted items drop out of old outfits.
  std::vector<qint64> itemIds;
};

/*!
 * \\brief Reads the outfits the user has chosen (the History tab).
 *
 * Choices are saved by RecommendationService::markOutfit().
 */
class HistoryRepository {
public:
  /*!
   * Chosen outfits whose recommendation time is in [from, to] (local time),
   * newest first.
   * \\throws DatabaseError if the database cannot be read.
   */
  static std::vector<HistoryEntry> chosenOutfits(const QDateTime &from,
                                                 const QDateTime &to);
};

#endif // HISTORYREPOSITORY_H
