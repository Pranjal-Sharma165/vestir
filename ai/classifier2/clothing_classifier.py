import torch

from ai.classifier2.base_classifier import BaseZeroShotClassifier

TAXONOMY = {
    "top": ["t-shirt", "blouse", "sweater", "hoodie", "shirt", "tank top"],
    "bottom": ["jeans", "trousers", "shorts", "skirt", "leggings"],
    "dress": ["dress", "jumpsuit", "romper"],
    "outerwear": ["jacket", "coat", "blazer", "parka", "cardigan", "vest"],
}
PLURAL = {"jeans", "trousers", "shorts", "leggings"}  # no "a" before these


class FlatClothingClassifier(BaseZeroShotClassifier):
    """One pass over all sub-category labels; the category is derived from them."""

    LABEL_NAMES = [n for subs in TAXONOMY.values() for n in subs]
    CATEGORY_OF = {n: c for c, subs in TAXONOMY.items() for n in subs}
    LABELS = [
        f"a photo of {n}" if n in PLURAL else f"a photo of a {n}"
        for n in LABEL_NAMES
    ]

    def predict_from_features(self, image_features) -> dict:
        with torch.no_grad():
            p = (self.logit_scale * image_features @ self.text_features.T).softmax(-1)[0]

        # category probability = sum of its sub-label probabilities
        cat_p = {}
        for i, name in enumerate(self.LABEL_NAMES):
            cat = self.CATEGORY_OF[name]
            cat_p[cat] = cat_p.get(cat, 0.0) + p[i].item()

        category = max(cat_p, key=cat_p.get)
        ranked = p.argsort(descending=True).tolist()

        # best sub-category *within the winning category*, so the two never disagree
        sub = next(i for i in ranked if self.CATEGORY_OF[self.LABEL_NAMES[i]] == category)

        return {
            "category": category,
            "category_confidence": round(cat_p[category], 4),
            "sub_category": self.LABEL_NAMES[sub],
            "sub_confidence": round(p[sub].item(), 4),
            "top_overall": self.LABEL_NAMES[ranked[0]],
            "category_scores": {k: round(v, 4) for k, v in cat_p.items()},
            "ranked": [
                {"label": self.LABEL_NAMES[i], "score": round(p[i].item(), 4)}
                for i in ranked[:5]
            ],
        }