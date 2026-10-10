from pathlib import Path

import open_clip
import torch
from PIL import Image


class ClothingClassifier:
    """Classify clothing images into Vestir's four clothing categories."""

    MODEL_NAME = "hf-hub:Marqo/marqo-fashionCLIP"

    LABELS = [
        "a photo of a top or upper body clothing item like t-shirt, blouse, sweater, hoodie",
        "a photo of a bottom or lower body clothing item like pants, jeans, skirt, shorts",
        "a photo of a dress, jumpsuit, romper or one-piece clothing",
        "a photo of outerwear like jacket, coat, blazer, parka, cardigan",
    ]

    LABEL_NAMES = ["top", "bottom", "dress", "outerwear"]

    def __init__(self):
        """Load FashionCLIP once when the classifier is created."""
        self.device = "cuda" if torch.cuda.is_available() else "cpu"

        self.model, _, self.preprocess = (
            open_clip.create_model_and_transforms(self.MODEL_NAME)
        )

        self.tokenizer = open_clip.get_tokenizer(self.MODEL_NAME)

        self.model = self.model.to(self.device)
        self.model.eval()

        text = self.tokenizer(self.LABELS).to(self.device)

        with torch.no_grad():
            self.text_features = self.model.encode_text(
                text,
                normalize=True,
            )

    def predict(self, image_path: str | Path) -> dict:
        """Predict the clothing category for one image."""
        image_path = Path(image_path)

        if not image_path.is_file():
            raise FileNotFoundError(
                f"Image not found: {image_path}"
            )

        with Image.open(image_path) as image:
            image_tensor = self.preprocess(
                image.convert("RGB")
            ).unsqueeze(0).to(self.device)

        with torch.no_grad():
            image_features = self.model.encode_image(
                image_tensor,
                normalize=True,
            )

            similarity = image_features @ self.text_features.T
            scores = similarity.softmax(dim=-1)

            predicted_index = scores.argmax(dim=-1).item()

        return {
            "category": self.LABEL_NAMES[predicted_index],
            "confidence": scores[0, predicted_index].item(),
            "scores": {
                label: scores[0, i].item()
                for i, label in enumerate(self.LABEL_NAMES)
            },
        }
