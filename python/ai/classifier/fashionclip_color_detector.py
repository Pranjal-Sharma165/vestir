from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from torch_polyfill_utils import softmax
import numpy as np


class FashionCLIPColorDetector:
    """Predict clothing colors using FashionCLIP."""

    COLORS = [
        "black", "white", "gray", "charcoal", "brown",
        "beige", "khaki", "cream", "navy blue", "blue",
        "light blue", "green", "olive green", "red", "pink",
        "purple", "yellow", "orange", "burgundy",
    ]

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        self.shared_model = shared_model or FashionCLIPModel()

        self.prompts = [
            f"a photo of a {color} colored clothing item"
            for color in self.COLORS
        ]
        self.text_features = self.shared_model.encode_text(self.prompts)

    def predict_from_features(self, image_features) -> dict:
        """Score already-encoded image features (no second vision pass)."""
        similarity = image_features @ self.text_features.T
        scores = softmax(similarity, axis=-1)[0]
        predicted_index = scores.argmax(axis=-1).item()

        ranked_indices = np.argsort(-scores, axis=-1).tolist()

        return {
            "color": self.COLORS[ranked_indices[0]],
            "confidence": round(scores[predicted_index].item(), 4),
            "ranked_colors": [
                {
                    "color": self.COLORS[i],
                    "score": round(scores[i].item(), 4),
                }
                for i in ranked_indices[:5]
            ],
        }

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)

        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")

        return self.predict_from_features(self.shared_model.encode_image(image_path))
