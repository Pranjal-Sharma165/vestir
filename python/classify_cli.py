"""
Command-line entry point used by the C++ backend to label a clothing photo.

Like recommend_cli.py, it exists because the Qt app cannot import Python code
and app2.py is a web server. The app starts this script once per photo:

    python classify_cli.py <image path>

stdout is one JSON object, the same fields as POST /classify in app2.py
(ClothingPipeline.predict_one): category_name, sub_category, primary_color,
formality, confidence{...}, low_confidence_formality. On failure it prints
{"error": "..."} and exits with code 1.

It adds no classification logic of its own. Needs torch and open_clip
(see requirements.txt); the first run downloads the FashionCLIP weights.
"""
import json
import sys

from ai.classifier2.pipeline import ClothingPipeline


def main():
    if len(sys.argv) != 2:
        print(json.dumps({"error": "usage: classify_cli.py <image path>"}))
        return 1
    try:
        result = ClothingPipeline().predict_one(sys.argv[1])
    except Exception as e:
        print(json.dumps({"error": str(e)}))
        return 1
    print(json.dumps(result))
    return 0


if __name__ == "__main__":
    sys.exit(main())
