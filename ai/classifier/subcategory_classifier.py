from ai.classifier.base_classifier import BaseZeroShotClassifier


class TopClassifier(BaseZeroShotClassifier):
    LABEL_NAMES = ["t-shirt", "blouse", "sweater", "hoodie", "shirt"]
    LABELS = [f"a photo of a {n}" for n in LABEL_NAMES]


class BottomClassifier(BaseZeroShotClassifier):
    LABEL_NAMES = ["jeans", "trousers", "shorts", "skirt", "leggings"]
    LABELS = [f"a photo of {n}" for n in LABEL_NAMES]


class OuterwearClassifier(BaseZeroShotClassifier):
    LABEL_NAMES = ["jacket", "coat", "blazer", "parka", "cardigan", "half-sleeve jacket"]
    LABELS = [f"a photo of a {n}" for n in LABEL_NAMES]


class DressClassifier(BaseZeroShotClassifier):
    LABEL_NAMES = ["dress", "jumpsuit", "romper"]
    LABELS = [f"a photo of a {n}" for n in LABEL_NAMES]