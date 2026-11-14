import copy
import time
from pathlib import Path

import open_clip
import torch
import torch.nn.functional as F
from PIL import Image

MODEL_NAME = "hf-hub:Marqo/marqo-fashionCLIP"
IMAGE_DIR = Path("wardrobe")
N_RUNS = 20

fp32, _, preprocess = open_clip.create_model_and_transforms(MODEL_NAME)
tokenizer = open_clip.get_tokenizer(MODEL_NAME)
fp32.eval()

# Half-precision copies of the image tower only; text stays fp32
variants = {
    "fp32": fp32.visual,
    "bf16": copy.deepcopy(fp32.visual).to(torch.bfloat16),
    "fp16": copy.deepcopy(fp32.visual).to(torch.float16),
}

paths = sorted(p for p in IMAGE_DIR.iterdir()
               if p.suffix.lower() in {".jpg", ".jpeg", ".png"})
batch = torch.stack([preprocess(Image.open(p).convert("RGB")) for p in paths])


def param_mb(visual):
    return sum(p.numel() * p.element_size() for p in visual.parameters()) / 1e6


def encode(visual, x):
    dtype = next(visual.parameters()).dtype
    with torch.inference_mode():
        out = visual(x.to(dtype))
        return F.normalize(out.float(), dim=-1)   # compare everything in fp32


def time_single(visual):
    x = batch[:1]
    encode(visual, x)                      # warm-up
    times = []
    for _ in range(N_RUNS):
        t = time.perf_counter()
        encode(visual, x)
        times.append(time.perf_counter() - t)
    times.sort()
    return times[len(times) // 2]          # median


# Speed + memory
for name, visual in variants.items():
    try:
        print(f"{name}: median {time_single(visual):.3f}s | weights {param_mb(visual):.0f} MB")
    except Exception as e:
        print(f"{name}: failed ({type(e).__name__}: {e})")

# Accuracy vs fp32
labels = ["shirt", "blouse", "jacket", "blazer", "jeans", "dress"]  # use your real prompts
with torch.inference_mode():
    text = F.normalize(
        fp32.encode_text(tokenizer([f"a photo of a {l}" for l in labels])), dim=-1
    )

ref = encode(variants["fp32"], batch)
ref_labels = (ref @ text.T).argmax(-1)

for name in ("bf16", "fp16"):
    try:
        emb = encode(variants[name], batch)
    except Exception:
        continue
    cos = (ref * emb).sum(-1)
    agree = (ref_labels == (emb @ text.T).argmax(-1)).float().mean()
    print(f"\n{name} vs fp32")
    print("  cosine sim per image:", cos)
    print(f"  min cosine: {cos.min():.4f} | mean: {cos.mean():.4f}")
    print(f"  label agreement: {agree:.0%}")
    