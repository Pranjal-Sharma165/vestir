
from pathlib import Path

from fashionclip_model import FashionCLIPModel


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

        self.prompts = [
            f"a photo of a {color} colored clothing item"
            for color in self.COLORS
        ]
        self.text_features = self.shared_model.encode_text(self.prompts)

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)

        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")

        image_features = self.shared_model.encode_image(image_path)

        import torch

        with torch.no_grad():
            similarity = image_features @ self.text_features.T
            scores = similarity.softmax(dim=-1)[0]

        ranked_indices = scores.argsort(descending=True).tolist()

        return {
            "main_color": self.COLORS[ranked_indices[0]],
            "ranked_colors": [
                {
                    "color": self.COLORS[i],
                    "score": round(scores[i].item(), 4),
                }
                for i in ranked_indices[:5]
            ],
        }
