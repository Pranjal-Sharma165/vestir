
from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.clothing_classifier import ClothingClassifier
from ai.classifier.subcategory_classifier import TopClassifier, BottomClassifier, DressClassifier, OuterwearClassifier


class ClothingPipeline:
    def __init__(self, shared_model=None):
        model = shared_model or FashionCLIPModel()
        self.category = ClothingClassifier(model)
        self.model = model
        self.subs = {
            "top": TopClassifier(model),
            "bottom": BottomClassifier(model),
            "dress": DressClassifier(model),
            "outerwear": OuterwearClassifier(model),
        }

    def predict(self, image_path):
        features = self.model.encode_image(Path(image_path))
        cat = self.category.predict_from_features(features)
        sub = self.subs[cat["label"]].predict_from_features(features)
        return {"category": cat, "sub_category": sub}