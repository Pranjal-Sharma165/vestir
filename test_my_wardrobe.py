
from pathlib import Path

from ai.classifier.formality_detector import FormalityDetector
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.clothing_classifier import ClothingClassifier
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector

from ai.recommendation.ranker import get_recommendations

from ai.recommendation.scoring import (
    calculate_color_score,
    calculate_formality_score,
    calculate_temperature_score,
    calculate_rain_score,
    calculate_weather_score,
    calculate_event_score
)


# -----------------------------
# SETTINGS
# -----------------------------

WARDROBE_FOLDER = Path("wardrobe")

TEMPERATURE = 10
RAIN_PROBABILITY = 60
EVENT = "school"

# -----------------------------
# LOAD CLASSIFIERS
# -----------------------------

print("Loading FashionCLIP...")

shared_model = FashionCLIPModel()

category_detector = ClothingClassifier(shared_model)
color_detector = FashionCLIPColorDetector(shared_model)
formality_detector = FormalityDetector(shared_model)

print("FashionCLIP loaded!\n")


# -----------------------------
# LOAD WARDROBE IMAGES
# -----------------------------

image_extensions = {".jpg", ".jpeg", ".png"}

image_paths = sorted(
    [
        path for path in WARDROBE_FOLDER.iterdir()
        if path.suffix.lower() in image_extensions
    ],
    key=lambda path: path.name.lower()
)

if not image_paths:
    raise ValueError("No images found in the wardrobe folder.")

print(f"Found {len(image_paths)} wardrobe images.\n")


# -----------------------------
# CLASSIFY CLOTHING
# -----------------------------

clothing_items = []

for item_id, image_path in enumerate(image_paths, start=1):

    category_result = category_detector.predict(str(image_path))
    color_result = color_detector.predict(str(image_path))
    formality_result = formality_detector.predict(str(image_path))

    # Extract actual labels from the results
    category = category_result["category"]
    color = color_result["color"]
    formality = formality_result["formality"]

    clothing_item = {
        "clothing_item_id": item_id,
        "category_name": category,
        "item_name": image_path.stem,
        "primary_color": color,
        "formality": formality
    }

    clothing_items.append(clothing_item)

    print(f"Image: {image_path.name}")
    print(f"Category: {category}")
    print(f"Color: {color}")
    print(f"Formality: {formality}")
    print("-" * 40)

# -----------------------------
# GENERATE RECOMMENDATIONS
# -----------------------------

print("\nGenerating outfit recommendations...\n")

recommendations = get_recommendations(
    clothing_items=clothing_items,
    temperature=TEMPERATURE,
    rain_probability=RAIN_PROBABILITY,
    event=EVENT,
    limit=5
)


# -----------------------------
# DISPLAY RESULTS
# -----------------------------

print(f"Total wardrobe items: {len(clothing_items)}")
print(f"Recommendations returned: {len(recommendations)}")

print("\nTOP RECOMMENDED OUTFITS")
print("=" * 50)

for index, outfit in enumerate(recommendations, start=1):

    print(f"\nRank {index}")
    print(f"Final Score: {outfit['score']:.2f}")

    print("\nClothing items:")

    for item in outfit["items"]:
        print(
            f"- {item['item_name']} | "
            f"{item['category_name']} | "
            f"{item['primary_color']} | "
            f"{item['formality']}"
        )

    # Individual scoring components
    color_score = calculate_color_score(outfit)
    formality_score = calculate_formality_score(outfit)
    temperature_score = calculate_temperature_score(
        outfit, TEMPERATURE
    )
    rain_score = calculate_rain_score(
        outfit, RAIN_PROBABILITY
    )
    weather_score = calculate_weather_score(
        outfit, TEMPERATURE, RAIN_PROBABILITY
    )
    event_score = calculate_event_score(outfit, EVENT)

    print("\nScore Breakdown:")
    print(f"Color:       {color_score:.2f}")
    print(f"Formality:   {formality_score:.2f}")
    print(f"Temperature: {temperature_score:.2f}")
    print(f"Rain:        {rain_score:.2f}")
    print(f"Weather:     {weather_score:.2f}")
    print(f"Event:       {event_score:.2f}")

    print("-" * 50)
