#ifndef CALENDARSERVICE_H
#define CALENDARSERVICE_H

#include "CalendarEntry.h"
#include "Database.h"
#include "Errors.h"
#include "MlClient.h"
#include "OutfitFormality.h"
#include <map>
#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

// Reads the user's calendar from an iCal (.ics) link and keeps the upcoming events in the database so they work offline.
class CalendarService : public QObject {
  Q_OBJECT
public:
  explicit CalendarService(Database &database, MlClient &mlClient,
                           QObject *parent = nullptr);

  QString icsUrl() const;

  void saveCalendarLink(const QString &url);
  void disconnectCalendar();
  bool isCalendarConnected() const;

  // Reads iCal text and returns the events that overlap [from, to]
  std::vector<CalendarEntry> parse(const QByteArray &ics, const QDateTime &from,
                                   const QDateTime &to) const;

  bool save(const std::vector<CalendarEntry> &events, const QDateTime &from,
            const QDateTime &to);

  std::vector<CalendarEntry> eventsBetween(const QDateTime &from,
                                           const QDateTime &to) const;

  std::vector<CalendarEntry> eventsOn(const QDate &day) const;

  std::optional<CalendarEntry> eventById(qint64 id) const;

  std::vector<std::string> untypedTitles() const;

  void setEventTypes(const std::map<std::string, OutfitFormality::Value> &types);

  void fetch();

signals:
  void fetched(bool ok, const QString &error);

private:
  Database &m_database;
  MlClient &m_ml;
  QNetworkAccessManager m_network;
};

#endif // CALENDARSERVICE_H
