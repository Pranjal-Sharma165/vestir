#include "CalendarService.h"
#include "TestHelpers.h"

#include <QCoreApplication>
#include <iostream>

static const QDateTime kFrom(QDate(2026, 11, 1), QTime(0, 0));
static const QDateTime kTo(QDate(2026, 12, 31), QTime(23, 59));

static QByteArray calendar(const QString &events) {
  return ("BEGIN:VCALENDAR\r\nVERSION:2.0\r\n" + events +
          "END:VCALENDAR\r\n")
      .toUtf8();
}

static QString event(const QString &lines) {
  return "BEGIN:VEVENT\r\n" + lines + "END:VEVENT\r\n";
}

template <typename E, typename F> static bool throwsError(F &&fn) {
  try {
    fn();
  } catch (const E &) {
    return true;
  } catch (...) {
    return false;
  }
  return false;
}

struct Fixture {
  Database database{":memory:", readSchema()};
  MlClient ml;
  CalendarService calendar{database, ml};
  Fixture() { database.initialize(); }

  void addEvent(const char *name, const char *start, const char *end,
                const char *dressCode, const char *source = "manual",
                const char *googleId = nullptr) {
    if (dressCode) {
      database.execute(
          "INSERT INTO calendar_events (event_name, start_time, end_time, "
          "dress_code, calendar_source, google_event_id) VALUES (?, ?, ?, ?, "
          "?, ?)",
          {name, start, end, dressCode, source, googleId ? googleId : name});
    } else {
      database.execute(
          "INSERT INTO calendar_events (event_name, start_time, end_time, "
          "calendar_source, google_event_id) VALUES (?, ?, ?, ?, ?)",
          {name, start, end, source, googleId ? googleId : name});
    }
  }
};

static void testParseSingleEvent() {
  Fixture f;
  const auto entries = f.calendar.parse(
      calendar(event("UID:a1\r\nSUMMARY:Team lunch\\, downtown\r\n"
                     "LOCATION:Cafe\r\nDESCRIPTION:Bring laptop\r\n"
                     "DTSTART:20261107T170000Z\r\nDTEND:20261107T180000Z\r\n")),
      kFrom, kTo);
  check(entries.size() == 1, "parse finds one event");
  if (entries.size() != 1)
    return;
  const CalendarEntry &e = entries[0];
  check(e.uid == "a1", "parse keeps the uid");
  check(e.name == "Team lunch, downtown", "parse unescapes the title");
  check(e.location == "Cafe", "parse reads the location");
  check(e.notes == "Bring laptop", "parse reads the notes");
  check(e.start.toUTC() == QDateTime(QDate(2026, 11, 7), QTime(17, 0),
                                     QTimeZone::utc()),
        "parse reads the start time");
  check(e.start.secsTo(e.end) == 3600, "parse reads the end time");
  check(!e.allDay, "timed event is not all day");
}

static void testParseAllDayAndFolded() {
  Fixture f;
  const auto entries = f.calendar.parse(
      calendar(event("UID:d1\r\nSUMMARY:Holiday\r\n"
                     "DTSTART;VALUE=DATE:20261107\r\n") +
               event("UID:f1\r\nSUMMARY:A very long\r\n  title\r\n"
                     "DTSTART:20261108T170000Z\r\n")),
      kFrom, kTo);
  check(entries.size() == 2, "parse finds both events");
  if (entries.size() != 2)
    return;
  check(entries[0].allDay, "date-only event is all day");
  check(entries[0].start.secsTo(entries[0].end) == 24 * 3600,
        "all day event lasts one day");
  check(entries[1].name == "A very long title", "folded lines are joined");
}

static void testParseSkipsEvents() {
  Fixture f;
  const auto entries = f.calendar.parse(
      calendar(event("UID:in\r\nSUMMARY:Keep\r\n"
                     "DTSTART:20261107T170000Z\r\n") +
               event("UID:old\r\nSUMMARY:Too early\r\n"
                     "DTSTART:20200101T170000Z\r\n") +
               event("UID:late\r\nSUMMARY:Too late\r\n"
                     "DTSTART:20300101T170000Z\r\n") +
               event("UID:cx\r\nSUMMARY:Cancelled\r\n"
                     "DTSTART:20261108T170000Z\r\nSTATUS:CANCELLED\r\n") +
               event("UID:nostart\r\nSUMMARY:No start\r\n")),
      kFrom, kTo);
  check(entries.size() == 1, "only the valid event in range is kept");
  check(!entries.empty() && entries[0].name == "Keep", "the right one is kept");
  check(f.calendar.parse("garbage", kFrom, kTo).empty(),
        "garbage gives no events");
}

static void testParseSortsByStart() {
  Fixture f;
  const auto entries = f.calendar.parse(
      calendar(event("UID:b\r\nSUMMARY:Later\r\n"
                     "DTSTART:20261109T170000Z\r\n") +
               event("UID:a\r\nSUMMARY:Earlier\r\n"
                     "DTSTART:20261107T170000Z\r\n")),
      kFrom, kTo);
  check(entries.size() == 2 && entries[0].name == "Earlier" &&
            entries[1].name == "Later",
        "events are sorted by start time");
}

static void testParseWeeklyRecurrence() {
  Fixture f;
  const auto entries = f.calendar.parse(
      calendar(event("UID:w1\r\nSUMMARY:Class\r\n"
                     "DTSTART:20261102T170000Z\r\nDTEND:20261102T180000Z\r\n"
                     "RRULE:FREQ=WEEKLY;COUNT=3\r\n"
                     "EXDATE:20261109T170000Z\r\n")),
      kFrom, kTo);
  check(entries.size() == 2, "weekly rule gives 3 events minus the excluded");
  if (entries.size() != 2)
    return;
  check(entries[0].uid != entries[1].uid, "each occurrence has its own uid");
  check(entries[0].start.daysTo(entries[1].start) == 14,
        "the excluded week is skipped");
}

static void testCalendarLink() {
  Fixture f;
  check(!f.calendar.isCalendarConnected(), "not connected at first");
  check(throwsError<InvalidInputError>(
            [&] { f.calendar.saveCalendarLink("not a link"); }),
        "invalid link is rejected");
  check(throwsError<InvalidInputError>(
            [&] { f.calendar.saveCalendarLink("ftp://example.com/a.ics"); }),
        "unsupported scheme is rejected");

  f.calendar.saveCalendarLink("  https://example.com/a.ics  ");
  check(f.calendar.isCalendarConnected(), "valid link connects the calendar");
  check(f.calendar.icsUrl() == "https://example.com/a.ics",
        "link is saved without spaces");

  f.addEvent("Synced", "2026-11-07 09:00:00", "2026-11-07 10:00:00", "casual",
             "ics");
  f.addEvent("Mine", "2026-11-08 09:00:00", "2026-11-08 10:00:00", "casual",
             "manual");
  f.calendar.disconnectCalendar();
  check(!f.calendar.isCalendarConnected(), "disconnect removes the link");
  const auto left = f.calendar.eventsBetween(kFrom, kTo);
  check(left.size() == 1 && left[0].name == "Mine",
        "disconnect removes synced events but keeps manual ones");
}

static void testReadEvents() {
  Fixture f;
  f.addEvent("Interview", "2026-11-07 09:00:00", "2026-11-07 10:00:00",
             "formal");
  f.addEvent("Gym", "2026-11-08 18:00:00", "2026-11-08 19:00:00", "sporty");

  const auto all = f.calendar.eventsBetween(kFrom, kTo);
  check(all.size() == 2, "eventsBetween returns both events");
  if (all.size() != 2)
    return;
  check(all[0].name == "Interview" && all[1].name == "Gym",
        "events are ordered by start time");
  check(all[0].dressCode == OutfitFormality::Value::Formal,
        "dress code is read");

  const auto day = f.calendar.eventsOn(QDate(2026, 11, 8));
  check(day.size() == 1 && day[0].name == "Gym", "eventsOn returns that day");
  check(f.calendar.eventsOn(QDate(2026, 11, 20)).empty(),
        "eventsOn is empty for a free day");

  const auto found = f.calendar.eventById(all[0].id);
  check(found.has_value() && found->name == "Interview",
        "eventById finds the event");
  check(found && found->start == QDateTime(QDate(2026, 11, 7), QTime(9, 0)),
        "eventById reads the start time");
  check(!f.calendar.eventById(9999).has_value(),
        "eventById is empty for an unknown id");
}

static void testEventTypes() {
  Fixture f;
  f.addEvent("Coffee", "2026-11-07 09:00:00", "2026-11-07 10:00:00", nullptr);
  f.addEvent("Gala", "2026-11-08 09:00:00", "2026-11-08 10:00:00", "casual");

  const auto untyped = f.calendar.untypedTitles();
  check(untyped.size() == 1 && untyped[0] == "Coffee",
        "only events without a dress code are untyped");

  f.calendar.setEventTypes({{"Coffee", OutfitFormality::Value::SmartCasual},
                            {"Gala", OutfitFormality::Value::Formal}});
  check(f.calendar.untypedTitles().empty(), "setEventTypes fills the gap");
  const auto all = f.calendar.eventsBetween(kFrom, kTo);
  check(all.size() == 2 &&
            all[0].dressCode == OutfitFormality::Value::SmartCasual,
        "the untyped event gets the new dress code");
  check(all.size() == 2 && all[1].dressCode == OutfitFormality::Value::Casual,
        "an event that already has a dress code is not overwritten");
}

int main(int argc, char *argv[]) {
  QCoreApplication app(argc, argv);
  testParseSingleEvent();
  testParseAllDayAndFolded();
  testParseSkipsEvents();
  testParseSortsByStart();
  testParseWeeklyRecurrence();
  testCalendarLink();
  testReadEvents();
  testEventTypes();
  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
