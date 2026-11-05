from pathlib import Path

from PIL import Image

from ai.classifier2.fashionclip_model import FashionCLIPModel
from ai.classifier2.clothing_classifier import FlatClothingClassifier
from ai.classifier2.formality_detector import FormalityDetector
from ai.classifier2.fashionclip_color_detector import FashionCLIPColorDetector

class ClothingPipeline:
    def __init__(self, shared_model=None, use_detector=False):
        self.model = shared_model or FashionCLIPModel()
        self.clothing = FlatClothingClassifier(self.model)
        self.formality = FormalityDetector(self.model)
        self.color = FashionCLIPColorDetector(self.model)

    def predict(self, image_path, max_items=4):
        """All garments found in the photo (a list, one entry per garment)."""
        with Image.open(Path(image_path)) as im:
            image = im.convert("RGB")
        image.thumbnail((1024, 1024)) 

        items = [{
            "crop": image,
            "bbox": (0, 0, *image.size),
            "label": "clothing",
            "confidence": 1.0,
        }]

        feats = self.model.encode_images([it["crop"] for it in items])

        results = []
        for it, f in zip(items, feats):
            f = f.unsqueeze(0)
            clothing = self.clothing.predict_from_features(f)
            results.append({
                "bbox": it["bbox"],
                "detector_label": it["label"],
                "detector_confidence": round(it["confidence"], 3),
                "category": clothing["category"],
                "sub_category": clothing["sub_category"],
                "color": self.color.predict_from_features(f, image=it["crop"]),
                "formality": self.formality.predict_from_features(f, clothing["category"]),
                "details": clothing,
            })
        return results

    def predict_one(self, image_path):
        """Single-garment photos (wardrobe uploads): one flat result."""
        r = self.predict(image_path, max_items=1)[0]
        return {
            "category_name": r["category"],
            "sub_category": r["sub_category"],
            "primary_color": r["color"]["color"],
            "formality": r["formality"]["formality"],
            "confidence": {
                "category": r["details"]["category_confidence"],
                "sub_category": r["details"]["sub_confidence"],
                "color": r["color"]["confidence"],
                "formality": r["formality"]["confidence"],
            },
            "low_confidence_formality": r["formality"]["low_confidence"],
        }