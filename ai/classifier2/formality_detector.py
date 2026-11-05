from pathlib import Path

import torch

from ai.classifier2.fashionclip_model import FashionCLIPModel

FORMALITIES = ["casual", "smart-casual", "formal", "sporty"]

PROMPTS = {
    "top": [
        "a casual relaxed top such as a basic t-shirt, hoodie, sweatshirt or loose everyday knit",
        "a smart-casual neat top such as a polo shirt, clean button-down shirt or fine-knit sweater",
        "a formal elegant top such as a crisp dress shirt, silk blouse or tailored formal top",
        "a sporty athletic top designed for exercise, such as a performance t-shirt, gym tank, running shirt or sports jersey with athletic fabric",
    ],
    "bottom": [
        "casual everyday bottoms such as jeans, relaxed cotton trousers, denim shorts or a casual skirt",
        "smart-casual neat bottoms such as chinos, tailored trousers or a tidy midi skirt",
        "formal bottoms such as suit trousers, dress pants or an elegant pencil skirt",
        "sporty athletic bottoms designed for exercise, such as joggers, track pants, athletic shorts or performance leggings",
    ],
    "dress": [
        "a casual everyday dress such as a sundress, t-shirt dress, denim dress or simple floral summer dress",
        "a smart-casual dress such as a wrap dress, shirt dress or neat midi dress suitable for work or dinner",
        "a formal dress such as a cocktail dress, evening gown or elegant occasion dress",
        "a sporty athletic dress designed for sports, such as a tennis dress or performance sportswear dress",
    ],
    "outerwear": [
        "a casual jacket or outer layer such as a denim jacket, bomber, hoodie-style jacket or simple knit cardigan",
        "a smart-casual outer layer such as an unstructured blazer, light trench or neat casual jacket",
        "a formal outer layer such as a tailored blazer, suit jacket or elegant wool overcoat",
        "a sporty athletic outer layer such as a track jacket, windbreaker, performance zip-up or athletic vest",
    ],
}

SPORTY_MIN_MARGIN = 0.28
UNCERTAIN_MARGIN = 0.10

class FormalityDetector:
    """Predict formality using prompts specific to the garment category."""

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        self.shared_model = shared_model or FashionCLIPModel()
        self.logit_scale = self.shared_model.logit_scale
        self.text_features = {
            cat: self.shared_model.encode_text(prompts)
            for cat, prompts in PROMPTS.items()
        }

    def predict_from_features(self, image_features, category: str = "top") -> dict:
        feats = self.text_features.get(category, self.text_features["top"])

        with torch.no_grad():
            p = (self.logit_scale * image_features @ feats.T).softmax(-1)[0]

        ranked = p.argsort(descending=True).tolist()
        best, second = ranked[0], ranked[1]
        margin = p[best].item() - p[second].item()

        chosen = best
        if FORMALITIES[best] == "sporty" and margin < SPORTY_MIN_MARGIN:
            chosen = second 

        return {
            "formality": FORMALITIES[chosen],
            "confidence": round(p[chosen].item(), 4),
            "low_confidence": margin < UNCERTAIN_MARGIN,
            "ranked_formalities": [
                {"formality": FORMALITIES[i], "score": round(p[i].item(), 4)}
                for i in ranked
            ],
        }

    def predict(self, image_path: str | Path, category: str = "top") -> dict:
        image_path = Path(image_path)
        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")
        return self.predict_from_features(
            self.shared_model.encode_image(image_path), category
        )