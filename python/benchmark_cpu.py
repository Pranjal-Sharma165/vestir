
from pathlib import Path
from time import perf_counter

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.formality_detector import FormalityDetector


WARDROBE_FOLDER = Path("wardrobe")
IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png"}


def timed_call(function, *args):
    start = perf_counter()
    result = function(*args)
    elapsed = perf_counter() - start
    return result, elapsed


def main():
    print("=" * 60)
    print("VESTIR CPU PERFORMANCE BENCHMARK")
    print("=" * 60)

    # Load model
    start = perf_counter()

    shared_model = FashionCLIPModel()
    pipeline = ClothingPipeline(shared_model)
    color_detector = FashionCLIPColorDetector(shared_model)
    formality_detector = FormalityDetector(shared_model)

    loading_time = perf_counter() - start

    print(f"\nDevice: {shared_model.device}")
    print(f"Model loading time: {loading_time:.2f} seconds")

    # Find images
    if not WARDROBE_FOLDER.exists():
        raise FileNotFoundError(f"Folder not found: {WARDROBE_FOLDER}")

    images = sorted(
        [
            path for path in WARDROBE_FOLDER.iterdir()
            if path.suffix.lower() in IMAGE_EXTENSIONS
        ],
        key=lambda path: path.name.lower()
    )

    if not images:
        raise ValueError("No images found.")

    print(f"\nImages found: {len(images)}")

    # Benchmark each image
    category_times = []
    color_times = []
    formality_times = []
    total_times = []

    for index, image in enumerate(images, start=1):
        print(f"\nProcessing {index}/{len(images)}: {image.name}")

        start = perf_counter()

        _, category_time = timed_call(pipeline.predict, image)
        _, color_time = timed_call(color_detector.predict, image)
        _, formality_time = timed_call(formality_detector.predict, image)

        total_time = perf_counter() - start

        category_times.append(category_time)
        color_times.append(color_time)
        formality_times.append(formality_time)
        total_times.append(total_time)

        print(f"  Category + subcategory: {category_time:.3f}s")
        print(f"  Color:                  {color_time:.3f}s")
        print(f"  Formality:              {formality_time:.3f}s")
        print(f"  Total:                  {total_time:.3f}s")

    # Results
    print("\n" + "=" * 60)
    print("BENCHMARK RESULTS")
    print("=" * 60)

    print(f"Device:                  {shared_model.device}")
    print(f"Images processed:        {len(images)}")
    print(f"Model loading:           {loading_time:.2f}s")
    print(f"Category average:        {sum(category_times)/len(images):.3f}s")
    print(f"Color average:           {sum(color_times)/len(images):.3f}s")
    print(f"Formality average:       {sum(formality_times)/len(images):.3f}s")
    print(f"Average per image:       {sum(total_times)/len(images):.3f}s")
    print(f"Total processing time:   {sum(total_times):.2f}s")
    print("=" * 60)


if __name__ == "__main__":
    main()
