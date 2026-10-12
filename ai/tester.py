
from formality_detector import FormalityDetector
from fashionclip_model import FashionCLIPModel
from clothing_classifier import ClothingClassifier
from fashionclip_color_detector import FashionCLIPColorDetector

image_path = "ai/test_image.jpeg"

# Load FashionCLIP exactly once.
shared_model = FashionCLIPModel()

# Give both detectors the same model instance.
category_detector = ClothingClassifier(shared_model)
color_detector = FashionCLIPColorDetector(shared_model)
formality_detector = FormalityDetector(shared_model)

print("Category:", category_detector.predict(image_path), "\n")
print("Color:", color_detector.predict(image_path), "\n")
print("Formality:", formality_detector.predict(image_path), "\n")
