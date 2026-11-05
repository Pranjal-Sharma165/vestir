from pathlib import Path

import open_clip
import torch
from PIL import Image


class FashionCLIPModel:
    """Shared FashionCLIP model for all clothing predictions."""

    MODEL_NAME = "hf-hub:Marqo/marqo-fashionCLIP"

    def __init__(self):
        self.device = "cuda" if torch.cuda.is_available() else "cpu"

        self.model, _, self.preprocess = (
            open_clip.create_model_and_transforms(self.MODEL_NAME)
        )
        self.tokenizer = open_clip.get_tokenizer(self.MODEL_NAME)

        self.model = self.model.to(self.device)
        self.model.eval()

        # CLIP's learned temperature (~100). Needed for meaningful softmax scores.
        self.logit_scale = self.model.logit_scale.exp().item()

    def encode_image(self, image):
        """Normalized features for one image (a file path or a PIL image)."""
        if isinstance(image, (str, Path)):
            with Image.open(image) as im:
                image = im.convert("RGB")

        tensor = self.preprocess(image.convert("RGB")).unsqueeze(0).to(self.device)

        with torch.no_grad():
            return self.model.encode_image(tensor, normalize=True)

    def encode_images(self, images):
        """Normalized features for many PIL images in one forward pass."""
        batch = torch.stack(
            [self.preprocess(i.convert("RGB")) for i in images]
        ).to(self.device)

        with torch.no_grad():
            return self.model.encode_image(batch, normalize=True)

    def encode_text(self, descriptions):
        """Normalized features for a list of text descriptions."""
        tokens = self.tokenizer(descriptions).to(self.device)

        with torch.no_grad():
            return self.model.encode_text(tokens, normalize=True)