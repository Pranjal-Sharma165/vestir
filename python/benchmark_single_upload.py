
from pathlib import Path
from time import perf_counter

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.formality_detector import FormalityDetector


IMAGE_PATH = Path("wardrobe/item1.jpeg")


def main():
    total_start = perf_counter()

    # 1. Load the model and classifiers
    print("Loading FashionCLIP...")

    start = perf_counter()

    shared_model = FashionCLIPModel()
    pipeline = ClothingPipeline(shared_model)
    color_detector = FashionCLIPColorDetector(shared_model)
    formality_detector = FormalityDetector(shared_model)

    loading_time = perf_counter() - start

    print(f"Device: {shared_model.device}")
    print(f"Model loading: {loading_time:.3f}s")

    # 2. Check image
    if not IMAGE_PATH.exists():
        raise FileNotFoundError(f"Image not found: {IMAGE_PATH}")

    # 3. Category and subcategory
    start = perf_counter()
    pipeline_result = pipeline.predict(IMAGE_PATH)
    category_time = perf_counter() - start

    # 4. Color
    start = perf_counter()
    color_result = color_detector.predict(IMAGE_PATH)
    color_time = perf_counter() - start

    # 5. Formality
    start = perf_counter()
    formality_result = formality_detector.predict(IMAGE_PATH)
    formality_time = perf_counter() - start

    # 6. Display predictions
    print("\nCLASSIFICATION RESULTS")
    print("-" * 45)

    print("Category:", pipeline_result["category"]["label"])
    print("Subcategory:", pipeline_result["sub_category"]["label"])
    print("Color:", color_result["color"])
    print("Formality:", formality_result["formality"])

    # 7. Timing summary
    total_time = perf_counter() - total_start
    classification_time = category_time + color_time + formality_time

    print("\nTIMING RESULTS")
    print("-" * 45)

    print(f"Model loading:             {loading_time:.3f}s")
    print(f"Category + subcategory:    {category_time:.3f}s")
    print(f"Color detection:            {color_time:.3f}s")
    print(f"Formality detection:        {formality_time:.3f}s")
    print(f"Classification total:       {classification_time:.3f}s")
    print(f"Total including loading:    {total_time:.3f}s")


if __name__ == "__main__":
    main()
