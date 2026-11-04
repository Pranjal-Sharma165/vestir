
from ai.classifier.formality_detector import FormalityDetector
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.clothing_classifier import ClothingClassifier
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector

image_path = "image.png"

# Load FashionCLIP exactly once.
shared_model = FashionCLIPModel()

# Give both detectors the same model instance.
category_detector = ClothingClassifier(shared_model)
color_detector = FashionCLIPColorDetector(shared_model)
formality_detector = FormalityDetector(shared_model)

print("Category:", category_detector.predict(image_path), "\n")
print("Color:", color_detector.predict(image_path), "\n")
print("Formality:", formality_detector.predict(image_path), "\n")
