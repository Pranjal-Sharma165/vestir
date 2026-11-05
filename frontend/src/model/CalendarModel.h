#ifndef CALENDARMODEL_H
#define CALENDARMODEL_H

#include <QObject>
#include <QProcess>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

/*!
 * \brief Upcoming calendar events for the Calendar page.
 *
 * Events come from an iCal link (Google/Outlook/Apple, synced by
 * ai-models/calendar_sync.py) or are added by hand. Both live in the
 * calendar_events table, which the recommender reads.
 */
class CalendarModel : public QObject {
  Q_OBJECT
  QML_ELEMENT

  Q_PROPERTY(QVariantList events READ events NOTIFY changed)
  Q_PROPERTY(QString icsUrl READ icsUrl NOTIFY changed)
  Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
  Q_PROPERTY(QString message READ message NOTIFY changed)

signals:
  void changed();
  void loadingChanged();

public:
  explicit CalendarModel(QObject *parent = nullptr);

  QVariantList events() const { return m_events; }
  QString icsUrl() const { return m_icsUrl; }
  bool loading() const { return m_loading; }
  QString message() const { return m_message; }

  /*! Re-reads the next 7 days of events from the database. */
  Q_INVOKABLE void reload();
  /*! Saves the iCal link (if not empty) and downloads its events. */
  Q_INVOKABLE void sync(const QString &url = QString());
  /*! date "yyyy-MM-dd", time "HH:mm", formality casual/smart-casual/formal/sporty. */
  Q_INVOKABLE bool addEvent(const QString &name, const QString &date,
                            const QString &time, const QString &formality);
  Q_INVOKABLE void removeEvent(int id);

private:
  QVariantList m_events;
  QString m_icsUrl;
  QString m_message;
  bool m_loading = false;
};

#endif // CALENDARMODEL_H
