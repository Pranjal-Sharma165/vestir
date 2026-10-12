
from pathlib import Path
import torch

from fashionclip_model import FashionCLIPModel


class ClothingClassifier:
    """Classify clothing images into Vestir's four categories."""

    LABELS = [
        "a photo of a top or upper body clothing item like t-shirt, blouse, sweater, hoodie",
        "a photo of a bottom or lower body clothing item like pants, jeans, skirt, shorts",
        "a photo of a dress, jumpsuit, romper or one-piece clothing",
        "a photo of outerwear like jacket, coat, blazer, parka, cardigan",
    ]

    
    # LABELS = [
    #     "a photo of a casual or dressy upper-body clothing item, such as a t-shirt, blouse, shirt, sweater, or hoodie",
    #     "a photo of a lower-body clothing item, such as trousers, jeans, chinos, shorts, or a skirt",
    #     "a photo of a one-piece clothing item covering the upper and lower body, such as a dress, jumpsuit, or romper",
    #     "a photo of an outerwear clothing item worn over other clothes, such as a jacket, coat, blazer, parka, or cardigan",
    # ]


    LABEL_NAMES = ["top", "bottom", "dress", "outerwear"]

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        # Reuse the supplied model, or load one for standalone use.
        self.shared_model = shared_model or FashionCLIPModel()

        self.text_features = self.shared_model.encode_text(self.LABELS)

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)

        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")

        image_features = self.shared_model.encode_image(image_path)

        with torch.no_grad():
            similarity = image_features @ self.text_features.T
            scores = similarity.softmax(dim=-1)
            predicted_index = scores.argmax(dim=-1).item()

        ranked_indices = scores[0].argsort(descending=True)

        return {
            "category": self.LABEL_NAMES[predicted_index],
            "confidence": round(scores[0, predicted_index].item(), 4),
            "ranked_categories": [
                {
                "category": self.LABEL_NAMES[i],
                "score": round(scores[0, i].item(), 4)
                }
                for i in ranked_indices
            ],
        }
