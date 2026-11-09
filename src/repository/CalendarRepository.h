#ifndef CALENDARREPOSITORY_H
#define CALENDARREPOSITORY_H

#include "CalendarEvent.h"
#include <QtTypes>
#include <memory>
#include <vector>

class CalendarRepository {
public:
  CalendarRepository();
  static std::vector<std::unique_ptr<CalendarEvent>> list();
};

#endif // CALENDARREPOSITORY_H
