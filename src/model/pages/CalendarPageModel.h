#ifndef CALENDARPAGEMODEL_H
#define CALENDARPAGEMODEL_H

#include "../repository/CalendarService.h"
#include "../repository/Errors.h"
#include "../repository/EventFormality.h"
#include "../repository/MlClient.h"
#include "../repository/RecommendationService.h"
#include <QJsonObject>
#include "CalendarEvent.h"
#include <QDate>
#include <QQmlEngine>

/*!
 * \brief Data source for the Calendar page and the calendar link in Settings.
 *
 * Shows the saved events of selectedDate. saveLink() stores the iCal link and
 * downloads the events; the link itself is never exposed to QML.
 */
class CalendarPageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QList<CalendarEvent *> items READ items NOTIFY itemsListChanged)
  Q_PROPERTY(QDate selectedDate READ selectedDate WRITE setSelectedDate NOTIFY
                 selectedDateChanged)
  Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString status READ status NOTIFY statusChanged)

signals:
  /*!
   * Signal that the events in the calendar have changed.
   */
  void itemsListChanged();
  void selectedDateChanged();
  void connectedChanged();
  void busyChanged();
  void statusChanged();
  /*! The stored events changed (link saved, refreshed or disconnected). */
  void calendarChanged();

public:
  explicit CalendarPageModel(QObject *parent = nullptr)
      : QObject(parent), m_selectedDate(QDate::currentDate()),
        m_service(this), m_ml(this) {
    connect(&m_service, &CalendarService::fetched, this,
            &CalendarPageModel::onFetched);
    m_connected = CalendarService::isCalendarConnected();
    reload();
    if (m_connected)
      refresh();
  }

  QList<CalendarEvent *> items() const { return m_items; }
  QDate selectedDate() const { return m_selectedDate; }
  bool connected() const { return m_connected; }
  bool busy() const { return m_busy; }
  QString status() const { return m_status; }

  void setSelectedDate(const QDate &date) {
    if (!date.isValid() || date == m_selectedDate)
      return;
    m_selectedDate = date;
    emit selectedDateChanged();
    reload();
  }

  /*! Stores the iCal link and downloads its events. */
  Q_INVOKABLE void saveLink(const QString &url) {
    try {
      CalendarService::saveCalendarLink(url);
    } catch (const VestirError &e) {
      setStatus(e.message());
      return;
    }
    setConnected(true);
    refresh();
  }

  /*! Forgets the link and removes the events that came from it. */
  Q_INVOKABLE void disconnectCalendar() {
    try {
      CalendarService::disconnectCalendar();
    } catch (const VestirError &e) {
      setStatus(e.message());
      return;
    }
    setConnected(false);
    setStatus({});
    reload();
    emit calendarChanged();
  }

  /*! Downloads the calendar again. */
  Q_INVOKABLE void refresh() {
    if (m_busy)
      return;
    setStatus({});
    setBusy(true);
    m_service.fetch();
  }

private:
  void onFetched(bool ok, const QString &error) {
    setStatus(ok ? QString() : error);
    reload();
    const QStringList titles = ok ? CalendarService::untypedTitles()
                                  : QStringList();
    if (titles.isEmpty()) {
      setBusy(false);
      if (ok)
        emit calendarChanged();
      return;
    }
    // Ask the ML what kind of event each title is, for the formality.
    m_ml.eventTypes(titles, [this](bool ok, const QJsonObject &result,
                                   const QString &) {
      onEventTypes(ok, result);
    });
  }

  void onEventTypes(bool ok, const QJsonObject &result) {
    if (ok) {
      QHash<QString, QString> types;
      const QJsonObject map = result["types"].toObject();
      for (auto it = map.begin(); it != map.end(); ++it)
        types.insert(it.key(), it.value().toString());
      CalendarService::setEventTypes(types);
    }
    setBusy(false);
    reload();
    emit calendarChanged();
  }

public:
  /*! Reads the selected day's events again. */
  Q_INVOKABLE void reload() {
    for (CalendarEvent *old : std::as_const(m_items))
      old->deleteLater();
    m_items.clear();
    try {
      for (const CalendarEntry &e : CalendarService::eventsOn(m_selectedDate)) {
        m_items.append(new CalendarEvent(e.name, e.location, e.notes, e.start,
                                         e.end,
                                         formalityForEvent(e.dressCode, e.eventType),
                                         e.allDay,
                                         RecommendationService::chosenOutfitId(e.id),
                                         this, e.id));
      }
    } catch (const VestirError &e) {
      setStatus(e.message());
    }
    emit itemsListChanged();
  }

private:
  void setConnected(bool v) {
    if (m_connected == v)
      return;
    m_connected = v;
    emit connectedChanged();
  }
  void setBusy(bool v) {
    if (m_busy == v)
      return;
    m_busy = v;
    emit busyChanged();
  }
  void setStatus(const QString &s) {
    if (m_status == s)
      return;
    m_status = s;
    emit statusChanged();
  }

  QList<CalendarEvent *> m_items;
  QDate m_selectedDate;
  bool m_connected = false;
  bool m_busy = false;
  QString m_status;
  CalendarService m_service;
  MlClient m_ml;
};

#endif // CALENDARPAGEMODEL_H
