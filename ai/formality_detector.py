from pathlib import Path
import torch

from fashionclip_model import FashionCLIPModel

class FormalityDetector:
    """Predict the formality style of a clothing item using FashionCLIP."""

    FORMALITIES = ["casual", "smart-casual", "formal", "sporty"]

    # PROMPTS = [
    #     "a photo of casual everyday clothing, such as a t-shirt, hoodie, or jeans",
    #     "a photo of smart-casual clothing, such as chinos, a polo shirt, or a neat blazer",
    #     "a photo of formal clothing, such as a tailored suit, dress shirt, or formal trousers",
    #     "a photo of sporty athletic clothing, such as a tracksuit, running shirt, or sportswear",
    # ]

    
    PROMPTS = [
        "a photo of casual everyday clothing with a relaxed, informal style, such as t-shirts, hoodies, jeans, or relaxed-fit trousers",
        "a photo of smart-casual clothing combining a neat, polished appearance with a relaxed style, such as chinos, polo shirts, button-down shirts, or unstructured blazers",
        "a photo of formal clothing with a refined, structured, or tailored appearance, such as business suits, dress shirts, tailored trousers, or elegant formal dresses",
        "a photo of sporty athletic clothing designed for exercise or active movement, such as tracksuits, running shirts, athletic shorts, or performance sportswear",
    ]


    def __init__(self, shared_model: FashionCLIPModel | None = None):
        self.shared_model = shared_model or FashionCLIPModel()
        self.text_features = self.shared_model.encode_text(self.PROMPTS)

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)

        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")

        image_features = self.shared_model.encode_image(image_path)

        with torch.no_grad():
            similarity = image_features @ self.text_features.T
            scores = similarity.softmax(dim=-1)[0]
            predicted_index = scores.argmax(dim=-1).item()

        ranked_indices = scores.argsort(descending=True).tolist()

        return {
            "formality": self.FORMALITIES[ranked_indices[0]],
            "confidence": round(scores[predicted_index].item(),4),
            "ranked_formalities": [
                {
                    "formality": self.FORMALITIES[i],
                    "score": round(scores[i].item(), 4),
                }
                for i in ranked_indices
            ],
        }