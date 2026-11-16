"""Export FashionCLIP to ONNX format. Supposed to only run on a development machine."""
import os
from pathlib import Path

import json
import shutil
import sys
import open_clip
import torch
import onnx
import numpy as np
from PIL import Image

# This straight up doesn't work. What??
os.environ["HF_HOME"] = str(Path(__file__).resolve().parent.parent / "hf_cache")
from huggingface_hub import hf_hub_download

if len(sys.argv) != 2:
    print("Usage: python export_fashionclip.py <export directory>", sys.stderr)
    exit(1)

MODEL_NAME = "hf-hub:Marqo/marqo-fashionCLIP"
out = Path(sys.argv[1])
out.mkdir(parents=True, exist_ok=True)

model, _, preprocess = open_clip.create_model_and_transforms(MODEL_NAME)
model.eval()
tokenizer = open_clip.get_tokenizer(MODEL_NAME)

assert isinstance(tokenizer, open_clip.tokenizer.SimpleTokenizer), type(tokenizer)
cfg = open_clip.get_model_preprocess_cfg(model)
assert cfg.get("interpolation", "bicubic") == "bicubic", cfg
assert cfg.get("resize_mode", "shortest") == "shortest", cfg

size = cfg["size"]
size = int(size[0] if isinstance(size, (tuple, list)) else size)
context_length = int(getattr(tokenizer, "context_length", 77))


class ImageEncoder(torch.nn.Module):
    def __init__(self, m):
        super().__init__()
        self.m = m

    def forward(self, x):
        return self.m.encode_image(x)


class TextEncoder(torch.nn.Module):
    def __init__(self, m):
        super().__init__()
        self.m = m

    def forward(self, t):
        return self.m.encode_text(t)


with torch.no_grad():
    batch = torch.export.Dim("batch")
    torch.onnx.export(
        ImageEncoder(model).eval(), (torch.randn(2, 3, size, size),), str(out / "image.onnx"),
        input_names=["pixel_values"], output_names=["image_features"],
        dynamic_shapes={"x": {0: batch}},
    )
    torch.onnx.export(
        TextEncoder(model).eval(), (torch.randint(1, 1000, (2, context_length)),), str(out / "text.onnx"),
        input_names=["input_ids"], output_names=["text_features"],
        dynamic_shapes={"t": {0: batch}},
    )

onnx.checker.check_model(str(out / "text.onnx"))
onnx.checker.check_model(str(out / "image.onnx"))

scale = getattr(model, "logit_scale", None)
(out / "fashionclip.json").write_text(json.dumps({
    "logit_scale": scale.exp().item() if scale is not None else 100.0,
    "image_size": size,
    "mean": [float(v) for v in cfg["mean"]],
    "std": [float(v) for v in cfg["std"]],
    "context_length": context_length,
}, indent=2))
shutil.copy(hf_hub_download("openai/clip-vit-base-patch32", "tokenizer.json"), out / "tokenizer.json")
