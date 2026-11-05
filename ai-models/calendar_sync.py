"""Import events from an iCalendar (.ics) link into calendar_events.

Works with the "secret address in iCal format" of Google Calendar, plus
Outlook / Apple calendar links. No account or API key needed.

    python calendar_sync.py --db <vestir.db> [--url <ics link>] [--days 7]

The link is saved in app_settings (key 'ics_url') and reused when --url is
omitted. Prints JSON: {"imported": n} or {"error": "..."}.
"""
import argparse
import json
import sqlite3
import sys
import urllib.request
from datetime import date, datetime, time, timedelta

import icalendar
import recurring_ical_events

FORMAL = ["interview", "presentation", "wedding", "ceremony", "gala",
          "funeral", "court", "client", "board", "conference", "dinner"]
SMART = ["meeting", "work", "lunch", "class", "lecture", "office", "review",
         "networking", "date", "seminar", "appointment", "exam"]
SPORTY = ["gym", "workout", "run", "yoga", "hike", "soccer", "basketball",
          "swim", "training", "practice", "sport", "tennis", "climb"]
TYPE_BY_WORD = [("school", ["class", "lecture", "exam", "school", "lab",
                            "seminar"]),
                ("gym", SPORTY),
                ("party", ["party", "birthday", "wedding", "gala", "concert",
                           "club", "celebration"]),
                ("work", FORMAL + ["meeting", "work", "office", "review",
                                   "lunch"])]


def infer(name, notes):
    text = f"{name} {notes or ''}".lower()
    for code, words in (("sporty", SPORTY), ("formal", FORMAL),
                        ("smart-casual", SMART)):
        if any(w in text for w in words):
            dress = code
            break
    else:
        dress = "casual"
    etype = None
    for t, words in TYPE_BY_WORD:
        if any(w in text for w in words):
            etype = t
            break
    return dress, etype or "hangout"


def to_local(value):
    """Return a naive local datetime for a date or datetime value."""
    if isinstance(value, datetime):
        return value.astimezone().replace(tzinfo=None) if value.tzinfo else value
    return datetime.combine(value, time.min)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--db", required=True)
    p.add_argument("--url")
    p.add_argument("--days", type=int, default=7)
    a = p.parse_args()

    con = sqlite3.connect(a.db)
    con.execute("CREATE TABLE IF NOT EXISTS app_settings "
                "(key TEXT PRIMARY KEY, value TEXT)")
    if a.url:
        con.execute("INSERT OR REPLACE INTO app_settings VALUES ('ics_url', ?)",
                    (a.url.strip(),))
        con.commit()
    row = con.execute(
        "SELECT value FROM app_settings WHERE key='ics_url'").fetchone()
    url = row[0] if row else None
    if not url:
        print(json.dumps({"error": "No calendar link set."}))
        return
    url = url.replace("webcal://", "https://")

    try:
        if url.startswith(("http://", "https://")):
            req = urllib.request.Request(url, headers={"User-Agent": "Vestir"})
            with urllib.request.urlopen(req, timeout=15) as r:
                data = r.read()
        else:
            with open(url, "rb") as f:
                data = f.read()
        cal = icalendar.Calendar.from_ical(data)
        start = date.today()
        end = start + timedelta(days=a.days)
        events = recurring_ical_events.of(cal).between(start, end)
    except Exception as e:
        print(json.dumps({"error": f"Could not read calendar: {e}"}))
        return

    # Replace previously imported events in this window with the fresh copy.
    con.execute("DELETE FROM calendar_events WHERE calendar_source='ics' "
                "AND substr(start_time,1,10) >= ?", (start.isoformat(),))
    n = 0
    for ev in events:
        name = str(ev.get("SUMMARY", "Event"))
        notes = str(ev.get("DESCRIPTION", "")) or None
        where = str(ev.get("LOCATION", "")) or None
        s_raw = ev["DTSTART"].dt
        e_raw = ev["DTEND"].dt if "DTEND" in ev else None
        s = to_local(s_raw)
        if isinstance(s_raw, datetime):
            e = to_local(e_raw) if e_raw else None
        else:  # all-day event
            e = s.replace(hour=23, minute=59)
        if e and e < s:
            e = s
        dress, etype = infer(name, notes)
        uid = f"{ev.get('UID', name)}|{s.isoformat()}"
        con.execute(
            "INSERT OR REPLACE INTO calendar_events (event_name, event_type,"
            " location, inferred_dress_code, start_time, end_time, notes,"
            " google_event_id, calendar_source) VALUES (?,?,?,?,?,?,?,?, 'ics')",
            (name, etype, where, dress, s.strftime("%Y-%m-%dT%H:%M"),
             e.strftime("%Y-%m-%dT%H:%M") if e else None, notes, uid))
        n += 1
    con.commit()
    print(json.dumps({"imported": n}))


if __name__ == "__main__":
    sys.exit(main())
