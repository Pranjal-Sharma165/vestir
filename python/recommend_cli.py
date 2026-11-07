"""
Command-line entry point used by the C++ backend (src/repository/MlClient).

Why it exists: the Qt app cannot import Python code, and app2.py is a web
server (it also loads torch for the classifier). This script lets the app
start Python once per request, with no server and no torch:

    stdin  : JSON, same as POST /recommendations in app2.py
    stdout : {"event": ..., "outfits": [{"item_ids": [...], "score": ...}]}

It adds no recommendation logic of its own; it only calls
ai.recommendation2 (get_recommendations, map_event). The one extra input is
"event_title": a raw calendar title, mapped to a recommender event type when
"event" is not given. Needs only the standard library.

Try it:  python recommend_cli.py < request.json
"""
import json
import sys

from ai.recommendation2.event_mapper import map_event
from ai.recommendation2.ranker import get_recommendations


def main():
    req = json.load(sys.stdin)

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
        diversity=req.get("diversity", 0.0),
        include_breakdown=req.get("include_breakdown", False),
    )

    result = []
    for o in outfits:
        entry = {
            "item_ids": sorted(i["clothing_item_id"] for i in o["items"]),
            "score": o["score"],
        }
        if "breakdown" in o:
            entry["breakdown"] = o["breakdown"]
        result.append(entry)

    json.dump({"event": event, "outfits": result}, sys.stdout)


if __name__ == "__main__":
    main()
