from pathlib import Path

import torch
import numpy as np
from PIL import Image

from ai.classifier2.fashionclip_model import FashionCLIPModel


COLORS = [
    "black", "white", "gray", "charcoal", "brown",
    "beige", "khaki", "cream", "navy blue", "blue",
    "light blue", "green", "olive green", "red", "pink",
    "purple", "yellow", "orange", "burgundy",
]

COLOR_PROMPTS = {
    "black":       "a photo of black clothing, deep solid black color",
    "white":       "a photo of pure white or bright white clothing",
    "gray":        "a photo of medium gray or heather gray clothing, cool neutral tone",
    "charcoal":    "a photo of dark charcoal gray clothing, almost black but still gray",
    "brown":       "a photo of brown or chocolate brown clothing, warm earthy tone",
    "beige":       "a photo of beige or tan clothing, light warm neutral sand color",
    "khaki":       "a photo of khaki or light olive-tan clothing",
    "cream":       "a photo of cream or off-white clothing, soft ivory or warm white tone",
    "navy blue":   "a photo of navy blue or dark blue clothing",
    "blue":        "a photo of medium blue or royal blue clothing",
    "light blue":  "a photo of light blue or sky blue clothing, pale cool blue",
    "green":       "a photo of green clothing, medium or forest green",
    "olive green": "a photo of olive green or muted military green clothing",
    "red":         "a photo of red or bright red clothing",
    "pink":        "a photo of pink or light pink clothing",
    "purple":      "a photo of purple or violet clothing",
    "yellow":      "a photo of yellow or bright yellow clothing",
    "orange":      "a photo of orange or bright orange clothing",
    "burgundy":    "a photo of burgundy or dark wine-red clothing",
}

NEUTRALS = {"black", "white", "gray", "charcoal", "brown", "beige", "khaki", "cream"}


def _brightness_and_warmth(image: Image.Image) -> tuple[float, float]:
    """Return (brightness 0-1, warmth -1 to +1) from a PIL image."""
    img = image.convert("RGB").resize((64, 64), Image.BILINEAR)
    arr = np.asarray(img).astype(np.float32) / 255.0

    gray = 0.299 * arr[:, :, 0] + 0.587 * arr[:, :, 1] + 0.114 * arr[:, :, 2]
    brightness = float(gray.mean())

    warmth = float((arr[:, :, 0] - arr[:, :, 2]).mean())
    return brightness, warmth


class FashionCLIPColorDetector:
    """Predict clothing colors using FashionCLIP + neutral visual re-rank."""

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        self.shared_model = shared_model or FashionCLIPModel()
        self.logit_scale = self.shared_model.logit_scale

        self.prompts = [COLOR_PROMPTS[c] for c in COLORS]
        self.text_features = self.shared_model.encode_text(self.prompts)

        self.neutral_list = [c for c in COLORS if c in NEUTRALS]
        neutral_prompts = [COLOR_PROMPTS[c] for c in self.neutral_list]
        self.neutral_features = self.shared_model.encode_text(neutral_prompts)

    def _visual_neutral_scores(self, image: Image.Image) -> dict[str, float]:
        """
        Hand-crafted scores for neutrals based on brightness + warmth.
        Higher = better match.
        """
        brightness, warmth = _brightness_and_warmth(image)

        scores = {}
        scores["cream"]    = self._gauss(brightness, 0.64, 0.10) * self._gauss(warmth, 0.06, 0.08)
        scores["beige"]    = self._gauss(brightness, 0.58, 0.09) * self._gauss(warmth, 0.05, 0.08)
        scores["khaki"]    = self._gauss(brightness, 0.52, 0.10) * self._gauss(warmth, 0.04, 0.09)
        scores["white"]    = self._gauss(brightness, 0.88, 0.10) * self._gauss(warmth, 0.01, 0.07)
        scores["gray"]     = self._gauss(brightness, 0.50, 0.12) * self._gauss(warmth, -0.02, 0.07)
        scores["charcoal"] = self._gauss(brightness, 0.28, 0.11) * self._gauss(warmth, -0.02, 0.07)
        scores["brown"]    = self._gauss(brightness, 0.38, 0.11) * self._gauss(warmth, 0.10, 0.09)
        scores["black"]    = self._gauss(brightness, 0.12, 0.09) * self._gauss(warmth, 0.00, 0.07)
        return scores

    @staticmethod
    def _gauss(x, mu, sigma):
        return float(np.exp(-0.5 * ((x - mu) / sigma) ** 2))

    def predict_from_features(
        self,
        image_features,
        image: Image.Image | None = None,
    ) -> dict:
        with torch.no_grad():
            scores = (
                self.logit_scale * image_features @ self.text_features.T
            ).softmax(-1)[0]

        ranked = scores.argsort(descending=True).tolist()
        top_idx = ranked[0]
        top_color = COLORS[top_idx]
        top_score = scores[top_idx].item()
        second_score = scores[ranked[1]].item()
        margin = top_score - second_score

        used_second_stage = False

        if top_color in NEUTRALS and image is not None:
            neutral_clip = (
                self.logit_scale * image_features @ self.neutral_features.T
            ).softmax(-1)[0]

            visual = self._visual_neutral_scores(image)

            combined = {}
            for i, c in enumerate(self.neutral_list):
                combined[c] = 0.45 * neutral_clip[i].item() + 0.55 * visual.get(c, 0.0)

            top_color = max(combined, key=combined.get)
            top_score = combined[top_color]
            used_second_stage = True

            sorted_neutrals = sorted(combined.items(), key=lambda x: x[1], reverse=True)
            ranked_colors = [{"color": c, "score": round(s, 4)} for c, s in sorted_neutrals[:5]]

            brightness, warmth = _brightness_and_warmth(image)
            print(f"[color-debug] brightness={brightness:.3f} warmth={warmth:.3f} "
                f"margin={margin:.3f} used_second={used_second_stage} → {top_color} "
                f"| visual={ {k: round(v,3) for k,v in visual.items()} }")
        else:
            ranked_colors = [
                {"color": COLORS[i], "score": round(scores[i].item(), 4)}
                for i in ranked[:5]
            ]
            if image is not None and top_color in NEUTRALS:
                brightness, warmth = _brightness_and_warmth(image)
                print(f"[color-debug] brightness={brightness:.3f} warmth={warmth:.3f} "
                    f"margin={margin:.3f} used_second=False (margin too large) → {top_color}")

        return {
            "color": top_color,
            "confidence": round(top_score, 4),
            "ranked_colors": ranked_colors,
            "used_second_stage": used_second_stage,
            "original_margin": round(margin, 4),
        }

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)
        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")

        with Image.open(image_path) as im:
            image = im.convert("RGB")

        features = self.shared_model.encode_image(image)
        return self.predict_from_features(features, image=image)