#include "CalendarService.h"
#include "Database.h"
#include "EnumStringMappings.h"

#include <QDebug>
#include <QHash>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QTimeZone>
#include <QUrl>
#include <algorithm>

namespace {

const char *kStoreFormat = "yyyy-MM-dd HH:mm:ss";

struct Prop {
  QString name;
  QMap<QString, QString> params;
  QString value;
};

struct Dt {
  QDateTime when;
  QTimeZone zone;
  bool allDay = false;
  bool ok = false;
};

QString unescapeText(QString s) {
  s.replace("\\n", "\n").replace("\\N", "\n");
  s.replace("\\,", ",").replace("\\;", ";").replace("\\\\", "\\");
  return s;
}

// "DTSTART;TZID=America/Vancouver:20261107T090000" -> name, params, value
bool parseLine(const QString &line, Prop &p) {
  const int colon = line.indexOf(':');
  if (colon <= 0)
    return false;
  const QStringList head = line.left(colon).split(';');
  p.name = head.at(0).toUpper();
  p.params.clear();
  for (int i = 1; i < head.size(); ++i) {
    const int eq = head.at(i).indexOf('=');
    if (eq > 0)
      p.params.insert(head.at(i).left(eq).toUpper(),
                      head.at(i).mid(eq + 1).remove('"'));
  }
  p.value = line.mid(colon + 1);
  return true;
}

Dt parseDt(const QString &value, const QMap<QString, QString> &params) {
  Dt d;
  const QTimeZone local = QTimeZone::systemTimeZone();
  if (params.value("VALUE").toUpper() == "DATE" || value.size() == 8) {
    const QDate date = QDate::fromString(value.left(8), "yyyyMMdd");
    if (!date.isValid())
      return d;
    d.zone = local;
    d.when = QDateTime(date, QTime(0, 0), local);
    d.allDay = true;
    d.ok = true;
    return d;
  }

  const bool utc = value.endsWith('Z');
  const QDateTime naive = QDateTime::fromString(utc ? value.chopped(1) : value,
                                                "yyyyMMdd'T'HHmmss");
  if (!naive.isValid())
    return d;
  if (utc) {
    d.zone = QTimeZone::utc();
  } else {
    d.zone = local;
    if (params.contains("TZID")) {
      const QTimeZone tz(params.value("TZID").toUtf8());
      if (tz.isValid())
        d.zone = tz;
    }
  }
  d.when = QDateTime(naive.date(), naive.time(), d.zone);
  d.ok = d.when.isValid();
  return d;
}

QString occurrenceUid(const QString &uid, const QDateTime &start) {
  return uid + "@" + start.toUTC().toString("yyyyMMdd'T'HHmmss");
}

struct RawEvent {
  QList<Prop> props;

  const Prop *find(const char *name) const {
    for (const Prop &p : props)
      if (p.name == QLatin1String(name))
        return &p;
    return nullptr;
  }
};

struct Rule {
  QString freq;
  int interval = 1;
  int count = 0; // 0 = unlimited
  QDateTime until;
  QList<int> byDay; // Qt::DayOfWeek, 1 = Monday
};

Rule parseRule(const QString &value, const QTimeZone &zone) {
  Rule r;
  static const QHash<QString, int> days{{"MO", 1}, {"TU", 2}, {"WE", 3},
                                        {"TH", 4}, {"FR", 5}, {"SA", 6},
                                        {"SU", 7}};
  for (const QString &part : value.split(';')) {
    const int eq = part.indexOf('=');
    if (eq < 0)
      continue;
    const QString key = part.left(eq).toUpper();
    const QString val = part.mid(eq + 1);
    if (key == "FREQ")
      r.freq = val.toUpper();
    else if (key == "INTERVAL")
      r.interval = std::max(1, val.toInt());
    else if (key == "COUNT")
      r.count = val.toInt();
    else if (key == "UNTIL") {
      QMap<QString, QString> none;
      Dt u = parseDt(val, none);
      if (u.ok) {
        // A date-only UNTIL ends at the end of that day in the event's zone.
        r.until = u.allDay
                      ? QDateTime(u.when.date().addDays(1), QTime(0, 0), zone)
                      : u.when;
      }
    } else if (key == "BYDAY") {
      for (const QString &d : val.split(',')) {
        // Ignore a numeric prefix such as "2MO"; only plain weekdays are used.
        const int v = days.value(d.right(2).toUpper(), 0);
        if (v)
          r.byDay.append(v);
      }
    }
  }
  return r;
}

// Start times of every occurrence up to 'limit', in the event's own zone.
QList<QDateTime> occurrences(const Dt &first, const Rule &rule,
                             const QDateTime &limit) {
  QList<QDateTime> out;
  const QTime time = first.when.time();
  const QDate startDate = first.when.date();
  const int kMaxSteps = 5000;

  auto accept = [&](const QDate &date) {
    const QDateTime at(date, time, first.zone);
    if (!at.isValid() || at > limit)
      return false;
    if (rule.until.isValid() && at > rule.until)
      return false;
    out.append(at);
    return rule.count == 0 || out.size() < rule.count;
  };

  if (rule.freq == "DAILY") {
    for (int i = 0; i < kMaxSteps; ++i)
      if (!accept(startDate.addDays(qint64(i) * rule.interval)))
        break;
  } else if (rule.freq == "WEEKLY") {
    QList<int> days = rule.byDay;
    if (days.isEmpty())
      days.append(startDate.dayOfWeek());
    std::sort(days.begin(), days.end());
    const QDate weekStart = startDate.addDays(-(startDate.dayOfWeek() - 1));
    bool more = true;
    for (int w = 0; w < kMaxSteps && more; ++w) {
      for (int day : days) {
        const QDate date =
            weekStart.addDays(qint64(w) * rule.interval * 7 + (day - 1));
        if (date < startDate)
          continue;
        if (!accept(date)) {
          more = false;
          break;
        }
      }
    }
  } else {
    out.append(first.when);
  }
  return out;
}

QString dateTimeText(const QDateTime &t) {
  return t.toLocalTime().toString(kStoreFormat);
}

QDateTime fromStored(const QString &text) {
  return QDateTime::fromString(text, kStoreFormat);
}

} // namespace

CalendarService::CalendarService(Database &database, MlClient &mlClient,
                                 QObject *parent)
    : QObject(parent), m_database(database), m_ml(mlClient) {
  m_network.setTransferTimeout(15000);
}

QString CalendarService::icsUrl() const {
  return QString::fromStdString(m_database.setting("ics_url"));
}

void CalendarService::saveCalendarLink(const QString &url) {
  const QString link = url.trimmed();
  const QUrl parsed(link);
  const QString scheme = parsed.scheme().toLower();
  if (link.isEmpty() || !parsed.isValid() || parsed.host().isEmpty() ||
      (scheme != "https" && scheme != "http" && scheme != "webcal"))
    throw InvalidInputError("Enter a calendar link starting with https://");

  if (!m_database.setSetting("ics_url", link.toStdString()))
    throw DatabaseError("Cannot save the calendar link");
}

void CalendarService::disconnectCalendar() {
  if (!m_database.execute("BEGIN"))
    throw DatabaseError("Cannot disconnect the calendar");
  const bool deleted =
      m_database.execute("DELETE FROM app_settings WHERE key = 'ics_url'") &&
      m_database.execute(
          "DELETE FROM calendar_events WHERE calendar_source = 'ics'");
  if (!deleted || !m_database.execute("COMMIT")) {
    m_database.execute("ROLLBACK");
    throw DatabaseError("Cannot disconnect the calendar");
  }
}

bool CalendarService::isCalendarConnected() const {
  return !icsUrl().trimmed().isEmpty();
}

std::vector<CalendarEntry> CalendarService::parse(const QByteArray &ics,
                                                  const QDateTime &from,
                                                  const QDateTime &to) const {
  // Long lines are folded: a line starting with a space continues the last.
  QString text = QString::fromUtf8(ics);
  text.replace("\r\n", "\n").replace('\r', '\n');
  text.replace("\n ", "").replace("\n\t", "");

  QList<RawEvent> raws;
  RawEvent current;
  bool inEvent = false;
  for (const QString &line : text.split('\n')) {
    if (line.compare("BEGIN:VEVENT", Qt::CaseInsensitive) == 0) {
      inEvent = true;
      current = {};
    } else if (line.compare("END:VEVENT", Qt::CaseInsensitive) == 0) {
      if (inEvent)
        raws.append(current);
      inEvent = false;
    } else if (inEvent) {
      Prop p;
      if (parseLine(line, p))
        current.props.append(p);
    }
  }

  // Edited single occurrences of a recurring event replace the generated one.
  QSet<QString> overridden;
  for (const RawEvent &r : raws) {
    const Prop *uid = r.find("UID");
    const Prop *rid = r.find("RECURRENCE-ID");
    if (!uid || !rid)
      continue;
    const Dt d = parseDt(rid->value, rid->params);
    if (d.ok)
      overridden.insert(occurrenceUid(uid->value, d.when));
  }

  std::vector<CalendarEntry> result;
  QSet<QString> seen;

  for (const RawEvent &r : raws) {
    const Prop *uid = r.find("UID");
    const Prop *startProp = r.find("DTSTART");
    if (!uid || !startProp)
      continue;
    const Prop *status = r.find("STATUS");
    if (status && status->value.compare("CANCELLED", Qt::CaseInsensitive) == 0)
      continue;

    const Dt start = parseDt(startProp->value, startProp->params);
    if (!start.ok)
      continue;

    qint64 durationSecs = start.allDay ? 24 * 3600 : 0;
    if (const Prop *endProp = r.find("DTEND")) {
      const Dt end = parseDt(endProp->value, endProp->params);
      if (end.ok)
        durationSecs = start.when.secsTo(end.when);
    }
    if (durationSecs < 0)
      durationSecs = 0;

    QSet<qint64> excluded;
    for (const Prop &p : r.props) {
      if (p.name != "EXDATE")
        continue;
      for (const QString &v : p.value.split(',')) {
        const Dt d = parseDt(v, p.params);
        if (d.ok)
          excluded.insert(d.when.toMSecsSinceEpoch());
      }
    }

    const Prop *rrule = r.find("RRULE");
    const bool isOverride = r.find("RECURRENCE-ID") != nullptr;
    QList<QDateTime> starts;
    if (rrule && !isOverride)
      starts = occurrences(start, parseRule(rrule->value, start.zone), to);
    else
      starts.append(start.when);
    const bool recurring = rrule || isOverride;

    for (const QDateTime &s : std::as_const(starts)) {
      if (excluded.contains(s.toMSecsSinceEpoch()))
        continue;
      const QDateTime localStart = s.toLocalTime();
      const QDateTime localEnd = s.addSecs(durationSecs).toLocalTime();
      if (localEnd < from || localStart > to)
        continue;

      QString entryUid = uid->value;
      if (recurring) {
        const Prop *rid = r.find("RECURRENCE-ID");
        if (rid) {
          const Dt d = parseDt(rid->value, rid->params);
          entryUid = occurrenceUid(uid->value, d.ok ? d.when : s);
        } else {
          entryUid = occurrenceUid(uid->value, s);
          if (overridden.contains(entryUid))
            continue;
        }
      }
      if (seen.contains(entryUid))
        continue;
      seen.insert(entryUid);

      CalendarEntry e;
      e.uid = entryUid;
      if (const Prop *p = r.find("SUMMARY"))
        e.name = unescapeText(p->value);
      if (e.name.isEmpty())
        e.name = "(No title)";
      if (const Prop *p = r.find("LOCATION"))
        e.location = unescapeText(p->value);
      if (const Prop *p = r.find("DESCRIPTION"))
        e.notes = unescapeText(p->value);
      e.start = localStart;
      e.end = localEnd;
      e.allDay = start.allDay;
      result.push_back(e);
    }
  }

  std::sort(result.begin(), result.end(),
            [](const CalendarEntry &a, const CalendarEntry &b) {
              return a.start < b.start;
            });
  return result;
}

bool CalendarService::save(const std::vector<CalendarEntry> &events,
                           const QDateTime &from, const QDateTime &to) {
  std::vector<std::string> titles;
  for (const CalendarEntry &e : events)
    titles.push_back(e.name.toStdString());
  const std::vector<OutfitFormality::Value> dressCodes =
      m_ml.classifyEventsLocal(titles).get();

  if (!m_database.execute("BEGIN"))
    return false;

  QSet<QString> keep;
  for (std::size_t i = 0; i < events.size(); ++i) {
    const CalendarEntry &e = events[i];
    const bool saved = m_database.execute(
        "INSERT INTO calendar_events (event_name, location, start_time, "
        "end_time, notes, google_event_id, calendar_source, dress_code) "
        "VALUES (?, ?, ?, ?, ?, ?, 'ics', ?) "
        "ON CONFLICT(google_event_id) DO UPDATE SET "
        "event_name = excluded.event_name, location = excluded.location, "
        "start_time = excluded.start_time, end_time = excluded.end_time, "
        "notes = excluded.notes",
        {e.name.toStdString(), e.location.toStdString(),
         dateTimeText(e.start).toStdString(), dateTimeText(e.end).toStdString(),
         e.notes.toStdString(), e.uid.toStdString(),
         std::string(textFor(kFormalities, dressCodes[i]).value())});
    if (!saved) {
      qWarning() << "Cannot save calendar event";
      m_database.execute("ROLLBACK");
      return false;
    }
    keep.insert(e.uid);
  }

  // Events deleted from the calendar disappear here too.
  std::vector<std::string> remove;
  m_database.query(
      "SELECT event_id, google_event_id FROM calendar_events "
      "WHERE calendar_source = 'ics' AND start_time >= ? AND start_time <= ?",
      {dateTimeText(from).toStdString(), dateTimeText(to).toStdString()},
      [&](const Database::Row &row) {
        if (!keep.contains(QString::fromStdString(row[1].value_or(""))))
          remove.push_back(row[0].value_or(""));
      });
  for (const std::string &id : remove)
    m_database.execute("DELETE FROM calendar_events WHERE event_id = ?", {id});
  if (!m_database.execute("COMMIT")) {
    m_database.execute("ROLLBACK");
    return false;
  }
  return true;
}

std::vector<CalendarEntry> CalendarService::eventsOn(const QDate &day) const {
  try {
    return eventsBetween(QDateTime(day, QTime(0, 0)),
                         QDateTime(day.addDays(1), QTime(0, 0)));
  } catch (const DatabaseError &) {
    return {};
  }
}

namespace {
const char *kEntryColumns =
    "SELECT event_id, google_event_id, event_name, location, notes, "
    "start_time, end_time, dress_code FROM calendar_events ";

// Reads one row selected with kEntryColumns.
CalendarEntry entryFromRow(const Database::Row &row) {
  const auto text = [&](std::size_t i) {
    return QString::fromStdString(row[i].value_or(""));
  };
  CalendarEntry e;
  e.id = text(0).toLongLong();
  e.uid = text(1);
  e.name = text(2);
  e.location = text(3);
  e.notes = text(4);
  e.start = fromStored(text(5));
  e.end = fromStored(text(6));
  e.dressCode = valueFor(kFormalities, row[7].value_or("")).value();
  return e;
}
} // namespace

std::vector<CalendarEntry>
CalendarService::eventsBetween(const QDateTime &from,
                               const QDateTime &to) const {
  std::vector<CalendarEntry> out;
  const bool ok = m_database.query(
      std::string(kEntryColumns) +
          "WHERE start_time < ? AND (COALESCE(end_time, start_time) > ? "
          "OR start_time >= ?) ORDER BY start_time",
      {to.toString(kStoreFormat).toStdString(),
       from.toString(kStoreFormat).toStdString(),
       from.toString(kStoreFormat).toStdString()},
      [&](const Database::Row &row) { out.push_back(entryFromRow(row)); });
  if (!ok)
    throw DatabaseError("Cannot read calendar events");
  return out;
}

std::optional<CalendarEntry> CalendarService::eventById(qint64 id) const {
  std::optional<CalendarEntry> result;
  m_database.query(std::string(kEntryColumns) + "WHERE event_id = ?",
                   {std::to_string(id)},
                   [&](const Database::Row &row) {
                     result = entryFromRow(row);
                   });
  return result;
}

std::vector<std::string> CalendarService::untypedTitles() const {
  std::vector<std::string> titles;
  m_database.query("SELECT DISTINCT event_name FROM calendar_events "
                   "WHERE dress_code IS NULL",
                   {}, [&](const Database::Row &row) {
                     titles.push_back(row[0].value_or(""));
                   });
  return titles;
}

void CalendarService::setEventTypes(
    const std::map<std::string, OutfitFormality::Value> &types) {
  if (!m_database.execute("BEGIN"))
    return;
  for (const auto &[title, formality] : types)
    m_database.execute("UPDATE calendar_events SET dress_code = ? "
                       "WHERE event_name = ? AND dress_code IS NULL",
                       {std::string(textFor(kFormalities, formality).value()),
                        title});
  m_database.execute("COMMIT");
}

void CalendarService::fetch() {
  QString link = icsUrl().trimmed();
  if (link.isEmpty()) {
    emit fetched(false, "No calendar link set");
    return;
  }
  if (link.startsWith("webcal://", Qt::CaseInsensitive))
    link.replace(0, 9, "https://");

  const QDateTime from(QDate::currentDate(), QTime(0, 0));
  const QDateTime to(QDate::currentDate().addDays(30), QTime(23, 59, 59));

  QNetworkRequest request{QUrl(link)};
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                       QNetworkRequest::NoLessSafeRedirectPolicy);
  QNetworkReply *reply = m_network.get(request);
  connect(reply, &QNetworkReply::finished, this, [this, reply, from, to] {
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
      emit fetched(false, reply->errorString());
      return;
    }
    const QByteArray body = reply->readAll();
    if (!body.contains("BEGIN:VCALENDAR")) {
      emit fetched(false, "That link is not an iCal calendar");
      return;
    }
    if (!save(parse(body, from, to), from, to)) {
      emit fetched(false, "Cannot save calendar events");
      return;
    }
    emit fetched(true, {});
  });
}
