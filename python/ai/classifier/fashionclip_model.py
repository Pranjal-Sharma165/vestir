import sys
import os
import ctypes

# Ensure global visibility for C++ extensions
if hasattr(sys, "setdlopenflags"):
    sys.setdlopenflags(os.RTLD_GLOBAL | os.RTLD_LAZY)
    
from pathlib import Path
import numpy as np
import onnxruntime as ort
import json
from PIL import Image
from tokenizers import Tokenizer

class FashionCLIPModel:
    """Shared FashionCLIP model for all clothing predictions."""

    def __init__(self):
        d = Path(__file__).parent / "fashionclip"
        meta = json.loads((d / "fashionclip.json").read_text())
        cpu = ["CPUExecutionProvider"]
 
        self.image = ort.InferenceSession(str(d / "image.onnx"), providers=cpu)
        self.text = ort.InferenceSession(str(d / "text.onnx"), providers=cpu)
        self.tokenizer = Tokenizer.from_file(str(d / "tokenizer.json"))
        self.tokenizer.enable_truncation(max_length=meta["context_length"])
        self.tokenizer.enable_padding(length=meta["context_length"], pad_id=0)
        self.logit_scale = meta["logit_scale"]
        self.size = meta["image_size"]
        self.mean = np.array(meta["mean"], np.float32).reshape(3, 1, 1)
        self.std = np.array(meta["std"], np.float32).reshape(3, 1, 1)

    def encode_image(self, image):
        """Normalized visual features for one image (a file path or a PIL image)."""
        if isinstance(image, (str, Path)):
            with Image.open(image) as im:
                image = im.convert("RGB")
 
        # Replace the preprocessing step in torch's pipeline
        n = self.size
        image = image.convert("RGB")
        w0, h0 = image.size
        w, h = (n, int(n * h0 / w0)) if w0 <= h0 else (int(n * w0 / h0), n)
        left, top = round((w - n) / 2), round((h - n) / 2)
        image = image.resize((w, h), Image.BICUBIC).crop((left, top, left + n, top + n))
        arr = np.asarray(image, np.float32).transpose(2, 0, 1) / 255
        batch = ((arr - self.mean) / self.std)[None]
        return self.__normalize(self.image.run(None, {"pixel_values": batch})[0])

    def encode_text(self, descriptions):
        """Convert text descriptions into normalized text features."""
        ids = np.array([e.ids for e in self.tokenizer.encode_batch(descriptions)], dtype=np.int64)
        return self.__normalize(self.text.run(None, {"input_ids": ids})[0])

    def __normalize(self, x):
        return x / np.linalg.norm(x, axis=-1, keepdims=True)
