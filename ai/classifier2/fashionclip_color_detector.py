from pathlib import Path

import torch

from ai.classifier2.fashionclip_model import FashionCLIPModel


class FashionCLIPColorDetector:
    """Predict clothing colors using FashionCLIP."""

    COLORS = [
        "black", "white", "gray", "charcoal", "brown",
        "beige", "khaki", "cream", "navy blue", "blue",
        "light blue", "green", "olive green", "red", "pink",
        "purple", "yellow", "orange", "burgundy",
    ]

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        # Reuse the supplied model, or load one for standalone use.
        self.shared_model = shared_model or FashionCLIPModel()
        self.logit_scale = self.shared_model.logit_scale

        self.prompts = [
            f"a photo of a {color} colored clothing item"
            for color in self.COLORS
        ]
        self.text_features = self.shared_model.encode_text(self.prompts)

    def predict_from_features(self, image_features) -> dict:
        with torch.no_grad():
            scores = (
                self.logit_scale * image_features @ self.text_features.T
            ).softmax(-1)[0]

        ranked = scores.argsort(descending=True).tolist()

        return {
            "color": self.COLORS[ranked[0]],
            "confidence": round(scores[ranked[0]].item(), 4),
            "ranked_colors": [
                {"color": self.COLORS[i], "score": round(scores[i].item(), 4)}
                for i in ranked[:5]
            ],
        }

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)
        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")
        return self.predict_from_features(self.shared_model.encode_image(image_path))