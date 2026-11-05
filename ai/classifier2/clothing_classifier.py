import torch

from ai.classifier2.base_classifier import BaseZeroShotClassifier

TAXONOMY = {
    "top": ["t-shirt", "blouse", "sweater", "hoodie", "shirt", "tank top"],
    "bottom": ["jeans", "trousers", "shorts", "skirt", "leggings"],
    "dress": ["dress", "jumpsuit", "romper"],
    "outerwear": ["jacket", "coat", "blazer", "parka", "cardigan", "vest"],
}
PLURAL = {"jeans", "trousers", "shorts", "leggings"}  # no "a" before these

LABEL_PROMPTS = {
    # Tops
    "t-shirt":   "a photo of a t-shirt or tee, short sleeves, casual knit top",
    "blouse":    "a photo of a blouse, softer or more feminine top",
    "sweater":   "a photo of a sweater or knit jumper, long sleeves, warmer knitwear",
    "hoodie":    "a photo of a hoodie or hooded sweatshirt",
    "shirt":     "a photo of a button-up shirt or button-down shirt, typically more structured",
    "tank top":  "a photo of a tank top or sleeveless top",

    # Bottoms
    "jeans":     "a photo of jeans or denim pants",
    "trousers":  "a photo of trousers or dress pants, not jeans",
    "shorts":    "a photo of shorts, short-length bottoms",
    "skirt":     "a photo of a skirt",
    "leggings":  "a photo of leggings or tight stretch pants",

    # Dresses 
    "dress":     "a photo of a dress, a one-piece garment covering both upper and lower body",
    "jumpsuit":  "a photo of a jumpsuit or one-piece outfit with pants",
    "romper":    "a photo of a romper or short one-piece outfit",

    # Outerwear
    "jacket":    "a photo of a jacket, waist or hip length outer layer",
    "coat":      "a photo of a coat, longer outerwear for warmth",
    "blazer":    "a photo of a blazer or structured suit-style jacket",
    "parka":     "a photo of a parka or hooded winter coat",
    "cardigan":  "a photo of a cardigan, open-front knit sweater",
    "vest":      "a photo of a vest or sleeveless outer layer",
}

class FlatClothingClassifier(BaseZeroShotClassifier):
    """One pass over all sub-category labels; the category is derived from them."""

    LABEL_NAMES = [n for subs in TAXONOMY.values() for n in subs]
    CATEGORY_OF = {n: c for c, subs in TAXONOMY.items() for n in subs}
    LABELS = [LABEL_PROMPTS[n] for n in LABEL_NAMES]

    def predict_from_features(self, image_features) -> dict:
        with torch.no_grad():
            p = (self.logit_scale * image_features @ self.text_features.T).softmax(-1)[0]

        cat_p = {}
        for i, name in enumerate(self.LABEL_NAMES):
            cat = self.CATEGORY_OF[name]
            cat_p[cat] = cat_p.get(cat, 0.0) + p[i].item()

        category = max(cat_p, key=cat_p.get)
        ranked = p.argsort(descending=True).tolist()

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