#ifndef CALENDARPAGEMODEL_H
#define CALENDARPAGEMODEL_H

#include "../repository/CalendarRepository.h"
#include "CalendarEvent.h"
#include <QQmlEngine>

class CalendarPageModel : public QObject {
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE("Owned by backend")
  Q_PROPERTY(QList<CalendarEvent *> items READ items NOTIFY itemsListChanged)

signals:
  /*!
   * Signal that the events in the calendar have changed.
   */
  void itemsListChanged();

public:
  explicit CalendarPageModel(QObject *parent = nullptr) : QObject(parent) {
    reload();
  }

  QList<CalendarEvent *> items() const { return m_items; }

private:
  void reload() {
    for (CalendarEvent *old : std::as_const(m_items))
      old->deleteLater();
    m_items.clear();
    for (auto &item : CalendarRepository::list()) {
      item->setParent(this);
      m_items.append(item.release());
    }
    emit itemsListChanged();
  }

  QList<CalendarEvent *> m_items;
};

#endif // CALENDARPAGEMODEL_H
