import torch

from ai.classifier.fashionclip_model import FashionCLIPModel


class ImageVerifier:
    """
    Zero-shot check: is this a photo of a supported clothing item?

    Shoes, bags and accessories are listed as separate groups only so the model
    has something to compare against. Anything that is not clothing is treated
    the same way by callers: an invalid image.

    Each group is scored by its best-matching prompt (max, not sum), so a group
    with more prompts has no advantage. Group scores become probabilities using
    CLIP's learned temperature.
    """

    GROUPS = {
        "clothing": [
            "a photo of a t-shirt",
            "a photo of a blouse",
            "a photo of a sweater",
            "a photo of a hoodie",
            "a photo of a shirt",
            "a photo of a tank top",
            "a photo of jeans",
            "a photo of trousers",
            "a photo of shorts",
            "a photo of a skirt",
            "a photo of leggings",
            "a photo of a dress",
            "a photo of a jumpsuit",
            "a photo of a jacket",
            "a photo of a coat",
            "a photo of a blazer",
            "a photo of a cardigan",
            "a photo of a person wearing an outfit",
            "a photo of folded clothes",
            "a photo of a garment on a hanger",
        ],
        "accessories": [
            "a photo of a pair of shoes",
            "a photo of sneakers",
            "a photo of boots",
            "a photo of a handbag",
            "a photo of a backpack",
            "a photo of a hat",
            "a photo of sunglasses",
            "a photo of jewelry",
            "a photo of a wristwatch",
            "a photo of a belt",
            "a photo of a scarf",
        ],
        "other": [
            "a photo of a landscape or scenery",
            "a photo of food on a plate",
            "a photo of a cat or a dog",
            "a photo of a car",
            "a photo of a building",
            "a close-up portrait of a face",
            "a screenshot of text or a document",
            "a photo of furniture in a room",
            "a photo of a laptop or a phone",
            "a photo of a plant or flowers",
            "a blurry or empty photo",
            "a cartoon or a drawing",
        ],
    }

    MIN_CLOTHING_PROB = 0.5

    def __init__(
        self,
        shared_model: FashionCLIPModel | None = None,
        min_clothing_prob: float | None = None,
    ):
        self.shared_model = shared_model or FashionCLIPModel()
        self.logit_scale = self.shared_model.logit_scale
        self.min_clothing_prob = (
            self.MIN_CLOTHING_PROB if min_clothing_prob is None else min_clothing_prob
        )

        self.group_names = list(self.GROUPS)
        prompts, self.slices = [], {}
        for group, group_prompts in self.GROUPS.items():
            start = len(prompts)
            prompts.extend(group_prompts)
            self.slices[group] = slice(start, len(prompts))

        self.text_features = self.shared_model.encode_text(prompts)

    def check_features(self, image_features) -> dict:
        """Verify already-encoded image features of shape (1, D)."""
        with torch.no_grad():
            logits = self.logit_scale * (image_features @ self.text_features.T)[0]
            group_logits = torch.stack(
                [logits[self.slices[g]].max() for g in self.group_names]
            )
            probs = group_logits.softmax(dim=-1)

        clothing_prob = probs[self.group_names.index("clothing")].item()

        return {
            "is_clothing": clothing_prob >= self.min_clothing_prob,
            "clothing_probability": round(clothing_prob, 4),
        }

    def check(self, image) -> dict:
        """Verify an image path or PIL image."""
        return self.check_features(self.shared_model.encode_image(image))