#include "CalendarService.h"
#include "Database.h"

#include <QDebug>
#include <QHash>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
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
  const QDateTime naive = QDateTime::fromString(
      utc ? value.chopped(1) : value, "yyyyMMdd'T'HHmmss");
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
        r.until = u.allDay ? QDateTime(u.when.date().addDays(1), QTime(0, 0),
                                       zone)
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

CalendarService::CalendarService(QObject *parent) : QObject(parent) {
  m_network.setTransferTimeout(15000);
}

QString CalendarService::icsUrl() { return Database::setting("ics_url"); }

void CalendarService::saveCalendarLink(const QString &url) {
  const QString link = url.trimmed();
  const QUrl parsed(link);
  const QString scheme = parsed.scheme().toLower();
  if (link.isEmpty() || !parsed.isValid() || parsed.host().isEmpty() ||
      (scheme != "https" && scheme != "http" && scheme != "webcal"))
    throw InvalidInputError("Enter a calendar link starting with https://");

  if (!Database::setSetting("ics_url", link))
    throw DatabaseError("Cannot save the calendar link");
}

void CalendarService::disconnectCalendar() {
  QSqlDatabase db = Database::connection();
  if (!db.transaction())
    throw DatabaseError("Cannot disconnect the calendar");
  QSqlQuery url(db);
  QSqlQuery events(db);
  if (!url.exec("DELETE FROM app_settings WHERE key = 'ics_url'") ||
      !events.exec("DELETE FROM calendar_events WHERE calendar_source = 'ics'")) {
    db.rollback();
    throw DatabaseError("Cannot disconnect the calendar");
  }
  if (!db.commit())
    throw DatabaseError("Cannot disconnect the calendar");
}

bool CalendarService::isCalendarConnected() {
  return !icsUrl().trimmed().isEmpty();
}

std::vector<CalendarEntry> CalendarService::parse(const QByteArray &ics,
                                                  const QDateTime &from,
                                                  const QDateTime &to) {
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
  QSqlDatabase db = Database::connection();
  if (!db.transaction())
    return false;

  QSqlQuery upsert(db);
  upsert.prepare(
      "INSERT INTO calendar_events (event_name, location, start_time, "
      "end_time, notes, google_event_id, calendar_source) "
      "VALUES (:name, :loc, :start, :end, :notes, :uid, 'ics') "
      "ON CONFLICT(google_event_id) DO UPDATE SET "
      "event_type = CASE WHEN event_name = excluded.event_name "
      "THEN event_type END, "
      "event_name = excluded.event_name, location = excluded.location, "
      "start_time = excluded.start_time, end_time = excluded.end_time, "
      "notes = excluded.notes");

  QSet<QString> keep;
  for (const CalendarEntry &e : events) {
    upsert.bindValue(":name", e.name);
    upsert.bindValue(":loc", e.location.isEmpty() ? QVariant() : e.location);
    upsert.bindValue(":start", dateTimeText(e.start));
    upsert.bindValue(":end", dateTimeText(e.end));
    upsert.bindValue(":notes", e.notes.isEmpty() ? QVariant() : e.notes);
    upsert.bindValue(":uid", e.uid);
    if (!upsert.exec()) {
      qWarning() << "Cannot save calendar event:" << upsert.lastError().text();
      db.rollback();
      return false;
    }
    keep.insert(e.uid);
  }

  // Events deleted from the calendar disappear here too.
  QSqlQuery stale(db);
  stale.prepare("SELECT event_id, google_event_id FROM calendar_events "
                "WHERE calendar_source = 'ics' AND start_time >= :from "
                "AND start_time <= :to");
  stale.bindValue(":from", dateTimeText(from));
  stale.bindValue(":to", dateTimeText(to));
  QList<qint64> remove;
  if (stale.exec())
    while (stale.next())
      if (!keep.contains(stale.value(1).toString()))
        remove.append(stale.value(0).toLongLong());
  for (qint64 id : std::as_const(remove)) {
    QSqlQuery del(db);
    del.prepare("DELETE FROM calendar_events WHERE event_id = :id");
    del.bindValue(":id", id);
    del.exec();
  }
  return db.commit();
}

std::vector<CalendarEntry> CalendarService::eventsOn(const QDate &day) {
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
    "start_time, end_time, event_type, "
    "COALESCE(dress_code, inferred_dress_code) FROM calendar_events ";

// Reads one row selected with kEntryColumns.
CalendarEntry entryFromRow(const QSqlQuery &q) {
  CalendarEntry e;
  e.id = q.value(0).toLongLong();
  e.uid = q.value(1).toString();
  e.name = q.value(2).toString();
  e.location = q.value(3).toString();
  e.notes = q.value(4).toString();
  e.start = fromStored(q.value(5).toString());
  e.end = fromStored(q.value(6).toString());
  e.eventType = q.value(7).toString();
  e.dressCode = q.value(8).toString();
  return e;
}
} // namespace

std::vector<CalendarEntry> CalendarService::eventsBetween(const QDateTime &from,
                                                          const QDateTime &to) {
  std::vector<CalendarEntry> out;
  QSqlQuery q(Database::connection());
  q.prepare(QString(kEntryColumns) +
            "WHERE start_time < :next AND (COALESCE(end_time, start_time) > "
            ":day OR start_time >= :day) ORDER BY start_time");
  q.bindValue(":day", from.toString(kStoreFormat));
  q.bindValue(":next", to.toString(kStoreFormat));
  if (!q.exec())
    throw DatabaseError("Cannot read calendar events");
  while (q.next())
    out.push_back(entryFromRow(q));
  return out;
}

std::optional<CalendarEntry> CalendarService::eventById(qint64 id) {
  QSqlQuery q(Database::connection());
  q.prepare(QString(kEntryColumns) + "WHERE event_id = :id");
  q.bindValue(":id", id);
  if (!q.exec() || !q.next())
    return std::nullopt;
  return entryFromRow(q);
}

QStringList CalendarService::untypedTitles() {
  QStringList titles;
  QSqlQuery q(Database::connection());
  if (q.exec("SELECT DISTINCT event_name FROM calendar_events "
             "WHERE event_type IS NULL OR event_type = ''"))
    while (q.next())
      titles.append(q.value(0).toString());
  return titles;
}

void CalendarService::setEventTypes(const QHash<QString, QString> &types) {
  QSqlDatabase db = Database::connection();
  if (!db.transaction())
    return;
  QSqlQuery q(db);
  q.prepare("UPDATE calendar_events SET event_type = :t "
            "WHERE event_name = :n AND (event_type IS NULL OR "
            "event_type = '')");
  for (auto it = types.cbegin(); it != types.cend(); ++it) {
    q.bindValue(":t", it.value());
    q.bindValue(":n", it.key());
    q.exec();
  }
  db.commit();
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
