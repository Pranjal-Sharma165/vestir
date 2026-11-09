#ifndef CALENDAREVENT_H
#define CALENDAREVENT_H

#include "OutfitFormality.h"
#include <QObject>
#include <QQmlEngine>

class CalendarEvent : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  Q_PROPERTY(QString qName READ qName CONSTANT)
  Q_PROPERTY(OutfitFormality::Value qFormality READ qFormality CONSTANT)
  Q_PROPERTY(bool allDay READ allDay CONSTANT)
  Q_PROPERTY(QString notes READ notes CONSTANT)
  Q_PROPERTY(QString location READ location CONSTANT)
  Q_PROPERTY(QDateTime startTime READ startTime CONSTANT)
  Q_PROPERTY(QDateTime endTime READ endTime CONSTANT)

signals:

public:
  explicit CalendarEvent(QString name, QString location, QString notes,
                         QDateTime startTime, QDateTime endTime,
                         OutfitFormality::Value formality, bool allDay,
                         int64_t assignedOutfitId = -1,
                         QObject *parent = nullptr)
      : QObject(parent), m_name(name), m_location(location), m_notes(notes),
        m_startTime(startTime), m_endTime(endTime), m_formality(formality),
        m_allDay(allDay), m_assignedOutfitId(assignedOutfitId) {}

  QString qName() const { return m_name; }
  OutfitFormality::Value qFormality() const { return m_formality; }
  QString location() const { return m_location; }
  QString notes() const { return m_notes; }
  QDateTime startTime() const { return m_startTime; }
  QDateTime endTime() const { return m_endTime; }
  bool allDay() const { return m_allDay; }

private:
  QString m_name;
  QString m_location;
  QString m_notes;
  QDateTime m_startTime;
  QDateTime m_endTime;
  OutfitFormality::Value m_formality;
  bool m_allDay;
  // -1 if no outfit has been assigned.
  int64_t m_assignedOutfitId;
};
#endif // CALENDAREVENT_H
