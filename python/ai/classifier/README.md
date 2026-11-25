# Vestir Clothing Classifier

Zero-shot clothing analysis for Vestir, built on [FashionCLIP](https://github.com/patrickjohncyh/fashion-clip). Given a photo, it works out whether the image is a clothing item, then predicts its **category**, **sub-category**, **colour** and **formality**. No task-specific training is needed: every label is a text prompt compared against the image embedding.

## How it works

One image is encoded **once**, and every classifier scores that same embedding against its own pre-encoded text prompts.

```
image ─► FashionCLIPModel.encode_image ─► image features (1 × D)
                                              │
        ┌───────────────┬──────────────┬──────┴───────┬───────────────┐
        ▼               ▼              ▼              ▼               ▼
  ImageVerifier   ClothingClassifier  Sub-category   ColorDetector  FormalityDetector
  (is clothing?)  (top/bottom/...)    classifier     (19 colours)   (4 styles)
                                      (picked by
                                       category)
```

Each classifier encodes its prompts once at construction time, then scoring a new image is just a matrix multiply and a softmax.

## Package layout

| File | Purpose |
|---|---|
| `fashionclip_model.py` | `FashionCLIPModel`: loads the ONNX image/text encoders and tokenizer, handles image preprocessing and normalisation. Shared by all classifiers. |
| `base_classifier.py` | `BaseZeroShotClassifier`: prompt encoding, softmax scoring and ranked output. Validates that `LABELS` and `LABEL_NAMES` match in length. |
| `clothing_classifier.py` | `ClothingClassifier`: stage 1, category (`top`, `bottom`, `dress`, `outerwear`). |
| `subcategory_classifier.py` | `TopClassifier`, `BottomClassifier`, `DressClassifier`, `OuterwearClassifier`: stage 2, sub-category within the predicted category. |
| `fashionclip_color_detector.py` | `FashionCLIPColorDetector`: 19 colours, returns the top 5. |
| `formality_detector.py` | `FormalityDetector`: `casual`, `smart-casual`, `formal`, `sporty`. |
| `image_verifier.py` | `ImageVerifier`: rejects photos that are not clothing. |
| `pipeline.py` | `ClothingPipeline`: ties everything together (`analyze`, `predict`). |
| `evaluate_classifier.py`, `evaluate_color.py`, `evaluate_formality.py` | CLI scripts that score each classifier against a labelled CSV. |

## Labels

| Classifier | Labels |
|---|---|
| Category | top, bottom, dress, outerwear |
| Top | t-shirt, blouse, sweater, hoodie, shirt, tank top |
| Bottom | jeans, trousers, shorts, skirt, leggings |
| Dress | dress, jumpsuit, romper |
| Outerwear | jacket, coat, blazer, parka, cardigan, half-sleeve jacket, vest |
| Colour | black, white, gray, charcoal, brown, beige, khaki, cream, navy blue, blue, light blue, green, olive green, red, pink, purple, yellow, orange, burgundy |
| Formality | casual, smart-casual, formal, sporty |

## Setup

The model files must be present in `ai/classifier/fashionclip/`:

```
fashionclip/
├── fashionclip.json   # context_length, logit_scale, image_size, mean, std
├── image.onnx
├── text.onnx
└── tokenizer.json
```

Inference runs on `onnxruntime` with the CPU execution provider. The package imports `onnxruntime`, `tokenizers`, `numpy`, `Pillow` and the local `torch_polyfill_utils` module (for `softmax`). The evaluation scripts additionally need `pandas` and `scikit-learn`.

Run everything from the `python/` directory so that `ai.classifier...` imports resolve.

## Usage

### Full analysis (recommended)

```python
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

pipeline = ClothingPipeline(FashionCLIPModel())   # load the model once, reuse it
result = pipeline.analyze("uploads/IMG_1984.jpg")

if not result["ok"]:
    print(result["error"])        # "Invalid image"
else:
    print(result["category"]["label"])            # "top"
    print(result["sub_category"]["label"])        # "t-shirt"
    print(result["color"]["color"])               # "black"
    print(result["formality"]["formality"])       # "casual"
    print(result["clothing_probability"])         # 0.98
```

`analyze()` does the following:

1. Opens the file and converts it to RGB. If it can't be read (missing, corrupt, empty, a directory), it returns `{"ok": False, "error": "Invalid image"}`.
2. Downscales so the longest side is at most 1024 px (smaller images are not upscaled).
3. Runs the verifier. A non-clothing image returns the same `Invalid image` error and **none of the other classifiers run**.
4. Predicts category, then sub-category using the classifier matching that category, plus colour and formality.

Pass `verify=False` to skip the clothing check. `clothing_probability` is then `None`.

### Category and sub-category only

```python
pipeline.predict("uploads/IMG_1984.jpg")
# {"category": {...}, "sub_category": {...}}
```

`predict()` does no verification or downscaling and raises on unreadable files. It is intended for evaluation.

### Using a single classifier

```python
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector

detector = FashionCLIPColorDetector(model)    # share the model you already loaded
detector.predict("uploads/my_shirt.jpg")
```

Every classifier also exposes `predict_from_features(features)`, which scores an already-encoded image and avoids a second vision pass.

## Output format

Successful `analyze()` result:

```python
{
    "ok": True,
    "category": {
        "label": "top",
        "confidence": 0.95,
        "ranked": [{"label": "top", "score": 0.95}, {"label": "outerwear", "score": 0.03}, ...],
    },
    "sub_category": {"label": "t-shirt", "confidence": 0.90, "ranked": [...]},
    "color": {
        "color": "black",
        "confidence": 0.88,
        "ranked_colors": [{"color": "black", "score": 0.88}, ...],   # top 5
    },
    "formality": {
        "formality": "casual",
        "confidence": 0.92,
        "ranked_formalities": [{"formality": "casual", "score": 0.92}, ...],
    },
    "clothing_probability": 0.98,
}
```

Scores are softmax probabilities rounded to 4 decimals. `ranked` lists are sorted from highest to lowest.

## How the image verifier works

Prompts are split into three groups: **clothing**, **accessories** (shoes, bags, hats, jewellery, ...) and **other** (landscapes, food, pets, screenshots, ...). The accessories and other groups exist only to give the model something to compare against.

- Each group is scored by its **best-matching prompt** (max, not sum), so a group with more prompts gets no advantage.
- The three group scores are turned into probabilities with a softmax using CLIP's learned `logit_scale`.
- The image is accepted if the clothing probability is at least `MIN_CLOTHING_PROB` (default `0.5`). Override it with `ImageVerifier(model, min_clothing_prob=0.7)`.

Shoes and accessories are rejected on purpose: the app only supports tops, bottoms, dresses and outerwear.

## Extending the classifiers

To add or change labels, edit the class lists. `LABELS` (the prompts) and `LABEL_NAMES` (what callers see) must have the same length and the same order, otherwise construction raises a `ValueError`.

```python
class BottomClassifier(BaseZeroShotClassifier):
    LABEL_NAMES = ["jeans", "trousers", "shorts", "skirt", "leggings"]
    LABELS = [f"a photo of {n}" for n in LABEL_NAMES]
```

If you add a new category to `ClothingClassifier`, add a matching sub-category classifier in `subcategory_classifier.py` and register it in `ClothingPipeline.subs`. A unit test enforces that every category has one.

## Testing

Tests live in `python/tests/`.

```bash
cd python
python -m pytest -v                        # fast unit tests, no model needed
RUN_MODEL_TESTS=1 python -m pytest -v      # also runs real-model accuracy tests
```

On Windows PowerShell:

```powershell
$env:RUN_MODEL_TESTS = "1"
python -m pytest -v
```

| File | What it covers |
|---|---|
| `test_classifier.py` | Pipeline behaviour using fake classifiers (invalid input, rejection, verification on/off). |
| `test_classifier_units.py` | Each classifier with a stub model: label reachability, sorted/normalised output, verifier groups and threshold, routing to the right sub-classifier, downscaling, non-RGB modes. |
| `test_classifier_samples.py` | Real FashionCLIP weights against `tests/data/samples.csv`. Checks accept/reject rates, category/sub-category/colour accuracy, determinism. Marked `model` and skipped unless `RUN_MODEL_TESTS=1`. |

Current accuracy thresholds for the sample tests: category ≥ 0.85, sub-category ≥ 0.60, colour ≥ 0.60, clothing acceptance rate ≥ 0.90.

## Evaluating accuracy

Each evaluation script reads a CSV and prints accuracy, a classification report and a confusion matrix, then saves per-image results.

```bash
python -m ai.classifier.evaluate_classifier --csv ai/evaluation.csv --images-dir ai
python -m ai.classifier.evaluate_color      --csv ai/evaluation.csv --images-dir ai
python -m ai.classifier.evaluate_formality  --csv ai/evaluation.csv --images-dir ai
```

| Argument | Default | Description |
|---|---|---|
| `--csv` | `ai/evaluation.csv` | Labelled evaluation file |
| `--images-dir` | `ai` | Folder containing the images |
| `--output` | `ai/<name>_evaluation_results.csv` | Where per-image predictions are written |

Required CSV columns:

| Script | Required columns |
|---|---|
| `evaluate_classifier.py` | `image`, `category` (optional: `sub_category`) |
| `evaluate_color.py` | `image`, `color` |
| `evaluate_formality.py` | `image`, `formality` |

Image names without an extension are assumed to be `.jpg`. With `sub_category` present, the classifier script reports both end-to-end sub-category accuracy and accuracy restricted to images whose category was predicted correctly, which separates stage-2 errors from stage-1 errors.

## Known limitations

- Predictions are zero-shot, so accuracy depends on prompt wording. Visually similar pairs (shirt vs. blouse, jacket vs. blazer, casual vs. smart-casual) are the most common confusions.
- Sub-category is only as good as the category: a wrong stage-1 prediction routes the image to the wrong sub-classifier.
- The colour and formality detectors apply softmax to raw cosine similarities, while the category classifiers and verifier scale by 100 (or the model's `logit_scale`) first. The predicted label is unaffected, but colour and formality `confidence` values are much flatter and are not directly comparable to category confidence.
- Each image is classified as a single garment; photos of a full outfit return one category and one colour.
- Inference is CPU-only through ONNX Runtime.