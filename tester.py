from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.formality_detector import FormalityDetector
from ai.classifier.pipeline import ClothingPipeline

IMAGE_PATH = Path("./uploads/IMG_1984.jpg")


def main():
    shared_model = FashionCLIPModel()

    pipeline = ClothingPipeline(shared_model)

    color_detector = FashionCLIPColorDetector(shared_model)
    formality_detector = FormalityDetector(shared_model)

    result = pipeline.predict(IMAGE_PATH)

    print("Category:", result["category"], "\n")
    print("Sub-category:", result["sub_category"], "\n")
    print("Color:", color_detector.predict(IMAGE_PATH), "\n")
    print("Formality:", formality_detector.predict(IMAGE_PATH), "\n")


if __name__ == "__main__":
    main()