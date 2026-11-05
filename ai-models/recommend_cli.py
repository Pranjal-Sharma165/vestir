"""Generate the next 5 outfit recommendations and print them as JSON.

Called by the Qt app:  python recommend_cli.py --db <vestir.db>

Steps: read wardrobe -> today's weather (Open-Meteo, no key) -> today's
calendar event (calendar_events table) -> rank -> save batch -> print JSON.
Outfits already shown today are excluded, so every call returns new ones.
"""
import argparse
import json
import sqlite3
import sys
import urllib.request
from datetime import datetime
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from ai.recommendation2.ranker import get_recommendations  # noqa: E402

EVENT_TYPES = {"school", "work", "party", "hangout", "gym"}
DRESS_CODE_TO_EVENT = {
    "formal": "work",
    "smart-casual": "work",
    "casual": "hangout",
    "sporty": "gym",
}
WEATHER_TEXT = {0: "Sunny", 1: "Mostly sunny", 2: "Partly cloudy", 3: "Cloudy",
                45: "Fog", 48: "Fog", 61: "Rain", 63: "Rain", 65: "Heavy rain",
                71: "Snow", 73: "Snow", 75: "Heavy snow", 80: "Showers",
                81: "Showers", 82: "Heavy showers", 95: "Thunderstorm"}


def fetch_weather(lat, lon, city):
    url = ("https://api.open-meteo.com/v1/forecast"
           f"?latitude={lat}&longitude={lon}&timezone=auto"
           "&current=temperature_2m,apparent_temperature,weather_code"
           "&daily=temperature_2m_max,temperature_2m_min,"
           "precipitation_probability_max")
    try:
        with urllib.request.urlopen(url, timeout=6) as r:
            d = json.load(r)
        cur, day = d["current"], d["daily"]
        code = cur.get("weather_code", 0)
        return {
            "city": city,
            "temperature": cur["temperature_2m"],
            "feels_like": cur.get("apparent_temperature"),
            "min": day["temperature_2m_min"][0],
            "max": day["temperature_2m_max"][0],
            "rain": (day["precipitation_probability_max"][0] or 0) / 100,
            "code": code,
            "cloudiness": WEATHER_TEXT.get(code, "Cloudy"),
            "offline": False,
        }
    except Exception as e:  # offline: keep the app usable
        print(f"weather unavailable: {e}", file=sys.stderr)
        return {"city": city, "temperature": 15.0, "feels_like": None,
                "min": 12.0, "max": 18.0, "rain": 0.1, "code": None,
                "cloudiness": "Unknown", "offline": True}


def todays_event(con):
    now = datetime.now()
    rows = con.execute(
        "SELECT event_id, event_name, event_type, "
        "COALESCE(dress_code, inferred_dress_code, 'casual') "
        "FROM calendar_events WHERE substr(start_time,1,10) = ? "
        "AND (end_time IS NULL OR end_time >= ?) ORDER BY start_time",
        (now.strftime("%Y-%m-%d"), now.strftime("%Y-%m-%dT%H:%M"))).fetchall()
    if not rows:
        return None, {"name": "No events today", "type": "hangout",
                      "formality": "casual"}
    # Dress for the most formal event still ahead today.
    rank = {"sporty": 0, "casual": 1, "smart-casual": 2, "formal": 3}
    eid, name, etype, code = max(rows, key=lambda r: rank.get(r[3], 1))
    etype = (etype or "").strip().lower()
    if etype not in EVENT_TYPES:
        etype = DRESS_CODE_TO_EVENT.get(code, "hangout")
    return eid, {"name": name, "type": etype, "formality": code}


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--db", required=True)
    p.add_argument("--lat", type=float, default=49.2827)
    p.add_argument("--lon", type=float, default=-123.1207)
    p.add_argument("--city", default="Vancouver")
    p.add_argument("--event", help="override event type (work, gym, ...)")
    p.add_argument("--limit", type=int, default=5)
    a = p.parse_args()

    con = sqlite3.connect(a.db)
    con.execute("PRAGMA foreign_keys = ON")

    rows = con.execute(
        "SELECT c.clothing_item_id, cat.category_name, c.item_name, "
        "c.primary_color, c.formality, c.sub_category, c.image_path "
        "FROM clothing_items c JOIN clothing_categories cat "
        "ON cat.category_id = c.category_id").fetchall()
    keys = ["clothing_item_id", "category_name", "item_name", "primary_color",
            "formality", "sub_category", "image_path"]
    items = [dict(zip(keys, r)) for r in rows]
    items = [i for i in items if i["primary_color"] and i["formality"]]

    weather = fetch_weather(a.lat, a.lon, a.city)
    event_id, event = todays_event(con)
    if a.event:
        event["type"] = a.event

    today = datetime.now().strftime("%Y-%m-%d")
    excluded = set()
    shown = con.execute(
        "SELECT o.recommendation_id, o.clothing_item_id FROM outfit_items o "
        "JOIN outfit_recommendations r USING (recommendation_id) "
        "WHERE substr(r.recommended_at,1,10) = ?", (today,)).fetchall()
    groups = {}
    for rid, cid in shown:
        groups.setdefault(rid, set()).add(cid)
    excluded = {frozenset(g) for g in groups.values()}

    outfits = get_recommendations(
        items, weather["temperature"], weather["rain"], event["type"],
        excluded_outfits=excluded, limit=a.limit, diversity=3.0)

    forecast_time = datetime.now().isoformat(timespec="minutes")
    con.execute(
        "INSERT OR REPLACE INTO weather_records (location, forecast_time,"
        " temperature_c, min_temp_c, max_temp_c, feels_like_c, cloudiness,"
        " weather_code, precipitation_probability) VALUES (?,?,?,?,?,?,?,?,?)",
        (weather["city"], forecast_time,
         weather["temperature"], weather["min"], weather["max"],
         weather["feels_like"], weather["cloudiness"], weather["code"],
         round(weather["rain"] * 100)))
    weather_id = con.execute(
        "SELECT weather_id FROM weather_records WHERE location=? AND "
        "forecast_time=?", (weather["city"], forecast_time)).fetchone()[0]
    batch_id = con.execute(
        "INSERT INTO recommendation_batches (event_id, weather_id) VALUES (?,?)",
        (event_id, weather_id)).lastrowid

    result = []
    outfits = sorted(outfits, key=lambda o: o["score"], reverse=True)
    for rank, o in enumerate(outfits, 1):
        rid = con.execute(
            "INSERT INTO outfit_recommendations (batch_id, rank, score)"
            " VALUES (?,?,?)", (batch_id, rank, o["score"])).lastrowid
        for it in o["items"]:
            con.execute("INSERT INTO outfit_items VALUES (?,?)",
                        (rid, it["clothing_item_id"]))
        result.append({
            "recommendation_id": rid,
            "score": o["score"],
            "items": [{"id": i["clothing_item_id"], "name": i["item_name"],
                       "category": i["category_name"],
                       "sub_category": i["sub_category"],
                       "color": i["primary_color"],
                       "image_path": i["image_path"]} for i in o["items"]],
        })
    con.commit()
    print(json.dumps({"batch_id": batch_id, "weather": weather,
                      "event": event, "outfits": result}))


if __name__ == "__main__":
    main()
