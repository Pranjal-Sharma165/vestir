from pathlib import Path

import torch

from ai.classifier2.fashionclip_model import FashionCLIPModel


class FormalityDetector:
    """Predict formality using prompts specific to the garment category."""

    FORMALITIES = ["casual", "smart-casual", "formal", "sporty"]

    # One prompt per entry in FORMALITIES (same order), per category.
    PROMPTS = {
        "top": [
            "a casual t-shirt, hoodie, sweatshirt or relaxed knit top",
            "a smart casual polo shirt, neat button-down shirt or fine-knit sweater",
            "a formal dress shirt, silk blouse or elegant tailored top",
            "a sporty athletic top, running shirt, gym tank or sports jersey",
        ],
        "bottom": [
            "casual jeans, relaxed cotton trousers, denim shorts or a casual skirt",
            "smart casual chinos, neat tailored trousers or a tidy midi skirt",
            "formal suit trousers, dress pants or an elegant pencil skirt",
            "sporty athletic shorts, joggers, track pants or sports leggings",
        ],
        "dress": [
            "a casual everyday dress, like a sundress, t-shirt dress, denim dress or floral summer dress",
            "a smart casual dress, like a wrap dress, shirt dress or neat midi dress",
            "a formal dress, like a cocktail dress, evening gown or elegant occasion dress",
            "a sporty athletic dress, like a tennis dress or performance sportswear dress",
        ],
        "outerwear": [
            "a casual jacket, denim jacket, bomber, hoodie-style jacket or knit cardigan",
            "a smart casual unstructured blazer, light trench coat or neat jacket",
            "a formal tailored blazer, suit jacket or elegant wool overcoat",
            "a sporty track jacket, windbreaker, athletic zip-up or performance vest",
        ],
    }

    # "sporty" needs a clear lead; otherwise fall back to the runner-up.
    SPORTY_MIN_MARGIN = 0.20
    # Below this lead between the top two, flag the result as low confidence.
    UNCERTAIN_MARGIN = 0.10

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        self.shared_model = shared_model or FashionCLIPModel()
        self.logit_scale = self.shared_model.logit_scale
        # encode once at startup, one block per category
        self.text_features = {
            cat: self.shared_model.encode_text(prompts)
            for cat, prompts in self.PROMPTS.items()
        }

    def predict_from_features(self, image_features, category: str = "top") -> dict:
        feats = self.text_features.get(category, self.text_features["top"])

        with torch.no_grad():
            p = (self.logit_scale * image_features @ feats.T).softmax(-1)[0]

        ranked = p.argsort(descending=True).tolist()
        best, second = ranked[0], ranked[1]
        margin = p[best].item() - p[second].item()

        chosen = best
        if self.FORMALITIES[best] == "sporty" and margin < self.SPORTY_MIN_MARGIN:
            chosen = second  # weak sporty win -> take the runner-up

        return {
            "formality": self.FORMALITIES[chosen],
            "confidence": round(p[chosen].item(), 4),
            "low_confidence": margin < self.UNCERTAIN_MARGIN,
            "ranked_formalities": [
                {"formality": self.FORMALITIES[i], "score": round(p[i].item(), 4)}
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