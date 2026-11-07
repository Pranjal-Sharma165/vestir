from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

from ai.recommendation2.ranker import get_recommendations


# --------------------------------------------------
# SETTINGS
# --------------------------------------------------

WARDROBE_FOLDER = Path("uploads")

TEMPERATURE = 10
RAIN_PROBABILITY = 20
EVENT = "work"

TOP_K = 5


# --------------------------------------------------
# LOAD CLASSIFIERS
# --------------------------------------------------

print("Loading FashionCLIP...")

shared_model = FashionCLIPModel()
pipeline = ClothingPipeline(shared_model)

print("FashionCLIP loaded!\n")


# --------------------------------------------------
# LOAD WARDROBE IMAGES
# --------------------------------------------------

image_extensions = {".jpg", ".jpeg", ".png"}

if not WARDROBE_FOLDER.exists():
    raise FileNotFoundError(
        f"Wardrobe folder not found: {WARDROBE_FOLDER}"
    )

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


# --------------------------------------------------
# CLASSIFY CLOTHING
# --------------------------------------------------

clothing_items = []
invalid_images = []
next_id = 1

print("=" * 55)
print("CLASSIFYING WARDROBE")
print("=" * 55)

for image_path in image_paths:

    result = pipeline.analyze(image_path)

    print(f"\nImage:       {image_path.name}")

    if not result["ok"]:
        invalid_images.append(image_path.name)
        print(f"Error:       {result['error']}")
        print("-" * 55)
        continue

    category = result["category"]["label"]
    subcategory = result["sub_category"]["label"]
    color = result["color"]["color"]
    formality = result["formality"]["formality"]

    clothing_items.append({
        "clothing_item_id": next_id,
        "item_name": image_path.stem,
        "category_name": category,
        "sub_category": subcategory,
        "primary_color": color,
        "formality": formality,
    })
    next_id += 1

    print(f"Category:    {category}")
    print(f"Subcategory: {subcategory}")
    print(f"Color:       {color}")
    print(f"Formality:   {formality}")

    print("-" * 55)

if invalid_images:
    print(f"\n{len(invalid_images)} invalid image(s): {', '.join(invalid_images)}")

if not clothing_items:
    raise SystemExit("\nNo valid clothing images found; nothing to recommend.")


# --------------------------------------------------
# GENERATE RECOMMENDATIONS
# --------------------------------------------------

print("\n" + "=" * 55)
print("GENERATING OUTFIT RECOMMENDATIONS")
print("=" * 55)

print(f"Temperature:      {TEMPERATURE}°C")
print(f"Rain probability: {RAIN_PROBABILITY}%")
print(f"Event:            {EVENT}")

recommendations = get_recommendations(
    clothing_items,
    temperature=TEMPERATURE,
    rain_probability=RAIN_PROBABILITY,
    event=EVENT,
    limit=TOP_K,
    include_breakdown=True
)


# --------------------------------------------------
# DISPLAY RESULTS
# --------------------------------------------------

print("\n" + "=" * 55)
print("TOP RECOMMENDED OUTFITS")
print("=" * 55)

print(f"\nTotal wardrobe items: {len(clothing_items)}")
print(f"Recommendations returned: {len(recommendations)}")

if not recommendations:
    print("\nNo valid outfits could be generated.")

else:
    for index, outfit in enumerate(recommendations, start=1):

        print(f"\nRANK {index}")
        print(f"Final Score: {outfit['score']:.2f}")

        print("\nClothing Items:")

        for item in outfit["items"]:
            print(
                f"- {item['item_name']} | "
                f"{item['category_name']} | "
                f"{item.get('sub_category', 'Unknown')} | "
                f"{item['primary_color']} | "
                f"{item['formality']}"
            )

        breakdown = outfit.get("breakdown")

        if breakdown:
            print("\nScore Breakdown:")

            for component, value in breakdown.items():
                if isinstance(value, (int, float)):
                    print(f"{component.replace('_', ' ').title():20}: {value:.2f}")
                else:
                    print(f"{component.replace('_', ' ').title():20}: {value}")

        print("\n" + "-" * 55)


print("\nWardrobe testing completed!")