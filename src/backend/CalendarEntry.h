#ifndef CALENDARENTRY_H
#define CALENDARENTRY_H

#include "OutfitFormality.h"
#include <QDateTime>
#include <QString>

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
  OutfitFormality::Value dressCode;
};

#endif // CALENDARENTRY_H
