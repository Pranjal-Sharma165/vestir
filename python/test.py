import json
import sys

# from ai.recommendation2.event_mapper import map_event
# from ai.recommendation2.ranker import get_recommendations

def test(string: str):
    return "Tested function call with string '" + string + "'"

def main():
    if req is None:
        print("Expected req to be set to request JSON.", sys.stderr)
        sys.exit(1);

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