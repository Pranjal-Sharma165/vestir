from pathlib import Path

from PIL import Image

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.clothing_classifier import ClothingClassifier
from ai.classifier.subcategory_classifier import TopClassifier, BottomClassifier, DressClassifier, OuterwearClassifier
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.formality_detector import FormalityDetector
from ai.classifier.image_verifier import ImageVerifier


def _invalid() -> dict:
    return {"ok": False, "error": "Invalid image"}


class ClothingPipeline:
    def __init__(self, shared_model=None, verify=True):
        model = shared_model or FashionCLIPModel()
        self.model = model
        self.category = ClothingClassifier(model)
        self.subs = {
            "top": TopClassifier(model),
            "bottom": BottomClassifier(model),
            "dress": DressClassifier(model),
            "outerwear": OuterwearClassifier(model),
        }
        self.color = FashionCLIPColorDetector(model)
        self.formality = FormalityDetector(model)
        self.verifier = ImageVerifier(model) if verify else None

    def predict(self, image_path):
        features = self.model.encode_image(Path(image_path))
        cat = self.category.predict_from_features(features)
        sub = self.subs[cat["label"]].predict_from_features(features)
        return {"category": cat, "sub_category": sub}

    def analyze(self, image_path) -> dict:
        """
        Success: {"ok": True, "category", "sub_category", "color", "formality",
                  "clothing_probability"}
        Failure: {"ok": False, "error": "Invalid image"}
                 (unreadable file, or the photo is not a clothing item)
        """
        try:
            with Image.open(Path(image_path)) as im:
                image = im.convert("RGB")
        except Exception:
            return _invalid()

        image.thumbnail((1024, 1024))
        f = self.model.encode_image(image)

        clothing_probability = None
        if self.verifier:
            v = self.verifier.check_features(f)
            if not v["is_clothing"]:
                return _invalid()
            clothing_probability = v["clothing_probability"]

        cat = self.category.predict_from_features(f)
        sub = self.subs[cat["label"]].predict_from_features(f)

        return {
            "ok": True,
            "category": cat,
            "sub_category": sub,
            "color": self.color.predict_from_features(f),
            "formality": self.formality.predict_from_features(f),
            "clothing_probability": clothing_probability,
        }