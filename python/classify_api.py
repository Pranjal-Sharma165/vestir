"""Module imported by call from the C API that contains functions related to image classification."""
import json
import sys

print("Loading weights...", flush=True)
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

pipeline = ClothingPipeline(FashionCLIPModel())
print("Done loading weights!", flush=True)

def classify(abs_path: str) -> str | None:
    """Classify an image. 

    Returns classes in the format '{category},{subcategory},{color},{formality}'. 
    See MlClient.cpp's parsePhotoClassificationResult for the decoding of this function's output.
    """
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
