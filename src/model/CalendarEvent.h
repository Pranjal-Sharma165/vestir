#ifndef CALENDAREVENT_H
#define CALENDAREVENT_H

#include "OutfitFormality.h"
#include <QObject>
#include <QQmlEngine>

class CalendarEvent : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")

  // CONSTANT for now; needs to change if the user is allowed to change these
  // values (likely).
  Q_PROPERTY(QString qName READ qName CONSTANT)
  Q_PROPERTY(OutfitFormality::Value qFormality READ qFormality CONSTANT)

signals:

public:
  explicit CalendarEvent(std::string name, std::string location,
                         std::time_t startTime, std::time_t endTime,
                         OutfitFormality::Value formality,
                         uint64_t assignedOutfitId = -1,
                         QObject *parent = nullptr)
      : QObject(parent), m_name(name), m_location(location),
        m_startTime(startTime), m_endTime(endTime), m_formality(formality),
        m_assignedOutfitId(assignedOutfitId) {}

  QString qName() const { return QString::fromStdString(m_name); }
  OutfitFormality::Value qFormality() const { return m_formality; }

private:
  std::string m_name;
  std::string m_location;
  std::time_t m_startTime;
  std::time_t m_endTime;
  OutfitFormality::Value m_formality;
  // -1 if no outfit has been assigned.
  uint64_t m_assignedOutfitId;
};
#endif // CALENDAREVENT_H
