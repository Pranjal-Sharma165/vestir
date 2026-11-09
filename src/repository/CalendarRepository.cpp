#include "CalendarRepository.h"

CalendarRepository::CalendarRepository() {}

std::vector<std::unique_ptr<CalendarEvent>> CalendarRepository::list() {
  std::vector<std::unique_ptr<CalendarEvent>> items;
  const QDateTime now = QDateTime::currentDateTime();
  // TODO replace placeholder data
  for (int i = 0; i < 10; i++) {
    const QDateTime start = now.addSecs(i * 3600);
    const QDateTime end = start.addSecs(3600);
    items.push_back(std::make_unique<CalendarEvent>(
        QString::fromStdString(std::format("Event {}", i)),
        QString::fromStdString(std::format("Location {}", i)),
        QString::fromStdString(std::format("Notes {}", i)), start, end,
        static_cast<OutfitFormality::Value>(
            i % static_cast<int>(OutfitFormality::Value::Sporty) + 1),
        static_cast<bool>(i % 2)));
  }

  return items;
}
