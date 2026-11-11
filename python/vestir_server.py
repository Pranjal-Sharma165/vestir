"""
ML server for the Android app (the phone cannot run Python).

    uvicorn vestir_server:app --host 0.0.0.0 --port 8001

It reuses app2.py (POST /analyze for photos, GET /health) and adds
POST /vestir/recommend, which takes the same JSON as recommend_cli.py
(event or event_title, ...) and returns {"event": ..., "outfits": [...]}.
The Android emulator reaches this PC at http://10.0.2.2:8001.
"""
from app2 import app
from ai.recommendation2.event_mapper import map_event
from ai.recommendation2.ranker import get_recommendations


@app.post("/vestir/event_types")
def vestir_event_types(req: dict):
    return {"types": {t: map_event(t).event for t in req.get("titles", [])}}


@app.post("/vestir/recommend")
def vestir_recommend(req: dict):
    event = req.get("event")
    if not event:
        event = map_event(req.get("event_title", "")).event
    excluded = {frozenset(ids) for ids in req.get("excluded_outfits", [])}
    outfits = get_recommendations(
        req["items"],
        req["temperature"],
        req["rain_probability"],
        event,
        excluded_outfits=excluded,
        limit=req.get("limit", 5),
    )
    return {
        "event": event,
        "outfits": [
            {
                "item_ids": sorted(i["clothing_item_id"] for i in o["items"]),
                "score": o["score"],
            }
            for o in outfits
        ],
    }
