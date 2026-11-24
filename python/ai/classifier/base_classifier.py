from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from torch_polyfill_utils import softmax
import numpy as np


class BaseZeroShotClassifier:
    LABELS: list[str] = []        # prompts sent to the text encoder
    LABEL_NAMES: list[str] = []   # names returned to callers

    def __init__(self, shared_model: FashionCLIPModel | None = None):
        if len(self.LABELS) != len(self.LABEL_NAMES):
            raise ValueError(f"{type(self).__name__}: LABELS and LABEL_NAMES must match in length")
        self.shared_model = shared_model or FashionCLIPModel()
        self.text_features = self.shared_model.encode_text(self.LABELS)

    def predict_from_features(self, image_features) -> dict:
        scores = softmax((100.0 * image_features @ self.text_features.T), axis=-1)

        ranked = np.argsort(-scores[0], axis=-1)
        best = ranked[0].item()
        return {
            "label": self.LABEL_NAMES[best],
            "confidence": round(scores[0, best].item(), 4),
            "ranked": [
                {"label": self.LABEL_NAMES[i], "score": round(scores[0, i].item(), 4)}
                for i in ranked
            ],
        }

    def predict(self, image_path: str | Path) -> dict:
        image_path = Path(image_path)
        if not image_path.is_file():
            raise FileNotFoundError(f"Image not found: {image_path}")
        return self.predict_from_features(self.shared_model.encode_image(image_path))
