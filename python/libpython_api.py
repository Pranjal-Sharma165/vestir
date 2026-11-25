"""Module imported by call from the C API that contains functions related to image classification."""
import json
import sys
import threading

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline
from ai.recommendation2.event_mapper import map_event
from ai.recommendation2.ranker import get_recommendations
from ai.recommendation2.scoring import EVENT_COMPATIBILITY

pipeline: ClothingPipeline | None = None
pipeline_import_done = threading.Event()

def import_clothing_pipeline():
    global pipeline
    print("Loading weights...", flush=True)
    pipeline = ClothingPipeline(FashionCLIPModel())
    pipeline_import_done.set()
    print("Done loading weights!", flush=True)

thread = threading.Thread(target=import_clothing_pipeline)
thread.start()

def classify(abs_path: str) -> str | None:
    """Classify an image. 

    Returns classes in the format '{category},{subcategory},{color},{formality}'. 
    See MlClient.cpp's parsePhotoClassificationResult for the decoding of this function's output.
    """
    _ = pipeline_import_done.wait()
    print(f"Classifying {abs_path}...")
    try:
        result = pipeline.analyze(abs_path)
    except Exception as e:
        print(json.dumps({"error": str(e)}), file=sys.stderr)
        return None
    if not result["ok"]:
        print(f"ERROR: {abs_path} {result['error']}\n", file=sys.stderr, flush=True)
        return None
    category =  result["category"]["label"],
    if isinstance(category, tuple):
        category = category[0]
    sub_category = result["sub_category"]["label"],
    if isinstance(sub_category, tuple):
        sub_category = sub_category[0]
    color = result["color"]["color"],
    if isinstance(color, tuple):
        color = color[0]
    formality = result["formality"]["formality"]
    if isinstance(formality, tuple):
        formality = formality[0]
    class_string = f"{category},{sub_category},{color},{formality}"
    return class_string

def formality_for_event(title: str) -> str | None:
    match = map_event(title)
    table = EVENT_COMPATIBILITY[match.event]
    ranked = sorted(table.items(), key=lambda kv: kv[1], reverse=True)
    if (len(ranked) == 0):
        return None
    return ranked[0][0]

def classify_events(titles: list[str]):
    return [formality_for_event(t) for t in titles]

def recommend(req):
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
