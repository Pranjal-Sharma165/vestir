from ai.classifier.base_classifier import BaseZeroShotClassifier


class ClothingClassifier(BaseZeroShotClassifier):

    LABEL_NAMES = ["top", "bottom", "dress", "outerwear"]

    # Descriptive prompts, one per entry in LABEL_NAMES (same order).
    LABELS = [
        "a photo of a top or upper body clothing item like t-shirt, blouse, sweater, hoodie, tank top",
        "a photo of a bottom or lower body clothing item like pants, jeans, skirt, shorts",
        "a photo of a dress, jumpsuit, romper or one-piece clothing",
        "a photo of outerwear like jacket, coat, blazer, parka, cardigan, vest",
    ]

    def _build_text_features(self):
        return self.shared_model.encode_text(self.LABELS)