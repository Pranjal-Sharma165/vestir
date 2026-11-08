#ifndef CALENDARSERVICE_H
#define CALENDARSERVICE_H

#include <QDate>
#include <QDateTime>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include "Errors.h"
#include <vector>

/*! One calendar event, as stored in calendar_events. */
struct CalendarEntry {
  qint64 id = 0;
  QString uid; // google_event_id column; unique per occurrence
  QString name;
  QString location;
  QString notes;
  QDateTime start; // local time
  QDateTime end;   // local time
  bool allDay = false;
  QString eventType; // school/work/party/hangout/gym, NULL until mapped
  QString dressCode; // casual/smart-casual/formal/sporty, NULL until known
};

/*!
 * \brief Reads the user's calendar from an iCal (.ics) link and keeps the
 * upcoming events in the database so they work offline.
 *
 * The link is stored in app_settings under "ics_url". In Google Calendar it
 * is "Settings > (your calendar) > Secret address in iCal format". webcal://
 * links are accepted too.
 *
 * Only events from today to 30 days ahead are kept.
 *
 * TODO later:
 *  - event_type is left empty. It should be filled by the ML team's
 *    map_event() when the recommendation bridge is built.
 *  - Recurring events: only DAILY and WEEKLY rules are expanded. MONTHLY and
 *    YEARLY events show only their first occurrence.
 *  - Android needs the INTERNET permission in the manifest.
 */
class CalendarService : public QObject {
  Q_OBJECT
public:
  explicit CalendarService(QObject *parent = nullptr);

  static QString icsUrl();

  /*!
   * Connects a calendar: validates and stores the link. Call fetch() after it
   * to download the events.
   * \throws InvalidInputError if the link is empty or not http(s)/webcal.
   * \throws DatabaseError if the link cannot be stored.
   */
  static void saveCalendarLink(const QString &url);

  /*!
   * Disconnects the calendar: forgets the link and deletes the events that
   * came from it.
   * \throws DatabaseError if the database cannot be written.
   */
  static void disconnectCalendar();

  /*! True if a calendar link is saved. Never exposes the link itself. */
  static bool isCalendarConnected();

  /*! Reads iCal text and returns the events that overlap [from, to]. */
  static std::vector<CalendarEntry>
  parse(const QByteArray &ics, const QDateTime &from, const QDateTime &to);

  /*!
   * Inserts or updates the given events. Events from the same link that are
   * inside [from, to] but no longer in the list are removed. Dress codes the
   * user has set are kept.
   */
  static bool save(const std::vector<CalendarEntry> &events,
                   const QDateTime &from, const QDateTime &to);

  /*!
   * Saved events that overlap [from, to], earliest first.
   * \throws DatabaseError if the database cannot be read.
   */
  static std::vector<CalendarEntry> eventsBetween(const QDateTime &from,
                                                  const QDateTime &to);

  /*! Saved events that overlap the given day, earliest first. */
  static std::vector<CalendarEntry> eventsOn(const QDate &day);

  /*! Downloads the calendar in the background and saves it. */
  void fetch();

signals:
  void fetched(bool ok, const QString &error);

private:
  QNetworkAccessManager m_network;
};

#endif // CALENDARSERVICE_H
