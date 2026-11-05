from pathlib import Path

from ai.classifier2.fashionclip_model import FashionCLIPModel
from ai.classifier2.pipeline import ClothingPipeline

IMAGE_PATH = Path("./uploads/IMG_1984.jpg")


def main():
    pipeline = ClothingPipeline(FashionCLIPModel())

    result = pipeline.predict_one(IMAGE_PATH)
    for key, value in result.items():
        print(f"{key}: {value}")

    # Full detail, useful when tuning prompts and margins
    print("\nFull results:")
    for item in pipeline.predict(IMAGE_PATH):
        print(item)


if __name__ == "__main__":
    main()