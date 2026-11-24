"""Module imported by call from the C API that contains functions related to image classification."""
import json
import sys

from ai.classifier.pipeline import ClothingPipeline

def classify(abs_path: str) -> str | None:
    print(f"Classifying {abs_path}...")
    try:
        result = ClothingPipeline().analyze(abs_path)
    except Exception as e:
        print(json.dumps({"error": str(e)}), file=sys.stderr)
        return None
    print(json.dumps(result))
    return json.dumps(result)
