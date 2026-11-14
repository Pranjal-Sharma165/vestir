import copy
import time
from pathlib import Path

import open_clip
import torch
import torch.nn.functional as F
from PIL import Image
from torch.ao.quantization import quantize_dynamic

MODEL_NAME = "hf-hub:Marqo/marqo-fashionCLIP"
IMAGE_DIR = Path("wardrobe")
N_RUNS = 20

fp32, _, preprocess = open_clip.create_model_and_transforms(MODEL_NAME)
tokenizer = open_clip.get_tokenizer(MODEL_NAME)
fp32.eval()

# Quantize only the image tower; text stays fp32
int8_visual = quantize_dynamic(
    copy.deepcopy(fp32.visual), {torch.nn.Linear}, dtype=torch.qint8
)

paths = sorted(p for p in IMAGE_DIR.iterdir()
               if p.suffix.lower() in {".jpg", ".jpeg", ".png"})
batch = torch.stack([preprocess(Image.open(p).convert("RGB")) for p in paths])

def encode(visual, x):
    with torch.inference_mode():
        return F.normalize(visual(x), dim=-1)

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

print(f"fp32 median: {time_single(fp32.visual):.3f}s")
print(f"int8 median: {time_single(int8_visual):.3f}s")

# Accuracy: embedding similarity + label agreement
f32 = encode(fp32.visual, batch)
i8 = encode(int8_visual, batch)
print("cosine sim per image:", (f32 * i8).sum(-1))

labels = ["shirt", "blouse", "jacket", "blazer", "jeans", "dress"]  # use your real prompts
with torch.inference_mode():
    text = F.normalize(
        fp32.encode_text(tokenizer([f"a photo of a {l}" for l in labels])), dim=-1
    )
agree = ((f32 @ text.T).argmax(-1) == (i8 @ text.T).argmax(-1)).float().mean()
print(f"label agreement: {agree:.0%}")
