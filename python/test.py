import json
import sys

from ai.classifier.pipeline import ClothingPipeline

def test(string: str):
    return "Tested function call with string '" + string + "'"

def classify(image_path: str) -> str | None:
    try:
        result = ClothingPipeline().predict(image_path)
    except Exception as e:
        print(json.dumps({"error": str(e)}), file=sys.stderr)
        return None
    print(json.dumps(result))
    return json.dumps(result)
