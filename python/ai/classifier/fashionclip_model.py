
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

    def encode_image(self, image_path):
        """Convert an image into normalized visual features."""
        with Image.open(image_path) as image:
            image_tensor = self.preprocess(
                image.convert("RGB")
            ).unsqueeze(0).to(self.device)

        with torch.no_grad():
            features = self.model.encode_image(
                image_tensor,
                normalize=True,
            )

        return features

    def encode_text(self, descriptions):
        """Convert text descriptions into normalized text features."""
        tokens = self.tokenizer(descriptions).to(self.device)

        with torch.no_grad():
            features = self.model.encode_text(
                tokens,
                normalize=True,
            )

        return features
