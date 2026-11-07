from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

IMAGE_PATH = Path("./uploads/people.jpg")


def main():
    pipeline = ClothingPipeline(FashionCLIPModel())

    result = pipeline.analyze(IMAGE_PATH)

    if not result["ok"]:
        print("Error:", result["error"])
        return

    print("Category:", result["category"], "\n")
    print("Sub-category:", result["sub_category"], "\n")
    print("Color:", result["color"], "\n")
    print("Formality:", result["formality"], "\n")
    print("Clothing probability:", result["clothing_probability"], "\n")


if __name__ == "__main__":
    main()