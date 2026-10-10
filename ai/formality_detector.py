from pathlib import Path

from fashionclip_model import FashionCLIPModel

class FormalityDetector:
    """Predict the formality style of a clothing item using FashionCLIP."""

    FORMALITIES = ["casual", "smart-casual", "formal", "sporty"]

    PROMPTS = [
        "a photo of casual everyday clothing, such as a t-shirt, hoodie, or jeans",
        "a photo of smart-casual clothing, such as chinos, a polo shirt, or a neat blazer",
        "a photo of formal clothing, such as a tailored suit, dress shirt, or formal trousers",
        "a photo of sporty athletic clothing, such as a tracksuit, running shirt, or sportswear",
    ]

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        self.shared_model = shared_model or FashionCLIPModel()
        self.text_features = self.shared_model.encode_text(self.PROMPTS)

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
            "formality": self.FORMALITIES[ranked_indices[0]],
            "ranked_formalities": [
                {
                    "formality": self.FORMALITIES[i],
                    "score": round(scores[i].item(), 4),
                }
                for i in ranked_indices
            ],
        }