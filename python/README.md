# Vestir: Python / AI Service

The Python side of Vestir, a wardrobe app. It does two jobs:

1. **Understand clothing photos.** Decide whether an image is a clothing item and predict its category, sub-category, colour and formality (zero-shot, using FashionCLIP).
2. **Recommend outfits.** Rank every valid outfit in a user's wardrobe for the weather and the event they are dressing for.

```
photo ──► ai/classifier ──► wardrobe item ──┐
                                            ├──► ai/recommendation2 ──► ranked outfits
weather (°C, rain %) + event title ─────────┘        (event title ► event_mapper)
```

## Folder structure

```
python/
├── ai/
│   ├── classifier/          Photo → category, sub-category, colour, formality
│   └── recommendation2/     Wardrobe + weather + event → ranked outfits
├── tests/                   Classifier tests (+ sample images and labels)
├── uploads/                 Local wardrobe photos used by the demo scripts
├── pytest.ini               Test configuration
├── requirements*.txt        Dependencies (see Setup)
│
├── vestir_server.py         Server entry point
├── app.py, app2.py          App / API entry points
├── torch_polyfill_utils.py  Small numpy replacement for torch helpers (softmax)
├── export_fashionclip.py    Exports FashionCLIP to the ONNX files the classifier loads
├── evaluate_classifier.py   Accuracy evaluation helper
├── compare_classifiers.py   Compares classifier versions
├── benchmark_*.py           Speed benchmarks for the classifier
├── wardrobe_demo*.py        End-to-end demo on uploads/ (classify, then recommend)
├── generator_demo.py,
│   ranker_demo.py,
│   recommendation_demo.py   Small printing demos of the recommender pieces
├── tester.py, tester2.py    Classify a single image and print the result
└── uploads_correct_classifications.csv   Ground-truth labels for uploads/
```
## Main components

### `ai/classifier/`: clothing photo analysis

`ClothingPipeline.analyze(path)` encodes the image once with FashionCLIP (ONNX, CPU) and scores that single embedding with several zero-shot classifiers:

| Stage | Output |
|---|---|
| Image verifier | Is it clothing? Shoes, accessories and non-clothing photos are rejected as `Invalid image`. |
| Category | `top`, `bottom`, `dress`, `outerwear` |
| Sub-category | e.g. `t-shirt`, `jeans`, `parka`, chosen by the predicted category |
| Colour | 19 colours |
| Formality | `casual`, `smart-casual`, `formal`, `sporty` |

```python
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

pipeline = ClothingPipeline(FashionCLIPModel())     # load once, reuse
result = pipeline.analyze("uploads/IMG_1984.jpg")
```

The model files live in `ai/classifier/fashionclip/` (`image.onnx`, `text.onnx`, `tokenizer.json`, `fashionclip.json`). Details: [`ai/classifier/README.md`](ai/classifier/README.md).

### `ai/recommendation2/`: outfit recommendation

A rule-based, explainable ranker. Outfits are Top + Bottom or Dress, each with optional Outerwear, and are scored from colour harmony, formality match, weather (warmth and rain) and event suitability.

```python
from ai.recommendation2.ranker import get_recommendations
from ai.recommendation2.event_mapper import map_event

event = map_event("Team stand-up").event            # "work"
recs = get_recommendations(items, temperature=10, rain_probability=60,
                           event=event, limit=5, include_breakdown=True)
```

`event_mapper` turns free-text calendar titles into one of `school`, `work`, `party`, `hangout`, `gym`. Details: [`ai/recommendation2/README.md`](ai/recommendation2/README.md).

### How the two connect

A classifier result becomes a wardrobe item for the recommender:

```python
{
    "clothing_item_id": 1,
    "item_name": "my_jeans",
    "category_name": result["category"]["label"],
    "sub_category": result["sub_category"]["label"],
    "primary_color": result["color"]["color"],
    "formality": result["formality"]["formality"],
}
```

`wardrobe_demo.py` runs this whole flow over the photos in `uploads/`.

## Setup

Python 3.11 is what the project is developed and tested on.

### Requirements files

There are four requirements files. They are separate on purpose (different targets and pins), so **don't merge or "tidy" them**.

| File | Used for | Notes |
|---|---|---|
| `requirements.txt` | Running the server and the AI code locally | FastAPI/uvicorn plus the inference stack (`onnxruntime`, `numpy`, `tokenizers`, `Pillow`) and `torch`/`open_clip_torch`. |
| `requirements-dev.txt` (hyphen) | Running the tests and evaluation scripts | `pytest`, `pandas`. |
| `requirements_dev.txt` (underscore) | Exporting FashionCLIP to ONNX (`export_fashionclip.py`) | CPU `torch`/`torchvision`, `open_clip_torch`, `onnx`, `onnxscript`. Includes `requirements_runtime.txt`. |
| `requirements_runtime.txt` | The Android build | Pinned to the wheels vendored in `android/wheels`. Replace those wheels before changing any version here. |

`requirements-dev.txt` and `requirements_dev.txt` look almost identical but are unrelated: one is test tooling, the other is the model-export environment.

### Local development and tests

```bash
cd python
python -m venv .venv
.venv\Scripts\activate                 # Windows
pip install -r requirements.txt
pip install -r requirements-dev.txt
```

### Exporting the ONNX model

Use a **separate virtual environment**, because `requirements_dev.txt` pulls in the Android pins from `requirements_runtime.txt` (for example a different `onnxruntime` and `pydantic` than `requirements.txt`).

```bash
pip install -r requirements_dev.txt
python export_fashionclip.py
```

### Things to know

- **Run everything from `python/`.** Imports are written as `ai.classifier...` and `ai.recommendation2...`, and `torch_polyfill_utils` is imported from the `python/` root. Running from another directory breaks them.
- Inference uses ONNX Runtime only. `torch` is still needed in the local environment because some unit tests build torch tensors.
- The evaluation scripts also need `scikit-learn`, which is not in `requirements-dev.txt`. Install it separately if you run them.
- The FashionCLIP ONNX files must exist in `ai/classifier/fashionclip/`. `export_fashionclip.py` generates them.

## Running things

```bash
python tester.py                              # classify one image
python wardrobe_demo.py                       # classify uploads/ and print top outfits
python -m ai.classifier.evaluate_classifier   # accuracy report (needs an evaluation CSV)
python -m ai.recommendation2.benchmark_recommender
```

## Tests

```bash
cd python
python -m pytest -v                          # fast tests, no model download
RUN_MODEL_TESTS=1 python -m pytest -v        # also real-model accuracy tests
```

PowerShell: `$env:RUN_MODEL_TESTS = "1"`.

| Location | Covers |
|---|---|
| `tests/test_classifier.py` | Pipeline behaviour with fake classifiers |
| `tests/test_classifier_units.py` | Each classifier with a stub model |
| `tests/test_classifier_samples.py` | Real model on `tests/data/samples/` against `samples.csv` (skipped unless `RUN_MODEL_TESTS=1`) |
| `ai/recommendation2/test_outfit_engine.py` | Fast engine equals brute-force reference; behaviour checks |
| `ai/recommendation2/test_event_mapper.py` | Calendar title mapping |

`pytest.ini` sets which folders are collected. The demo and script files at the `python/` root are deliberately **not** named `test_*.py`: they run at import time (loading the model, reading `uploads/`), so pytest must never collect them.

## Conventions

- New tests go in `tests/` (classifier, API) or next to the code they test (recommender).
- Any script that loads the model or reads `uploads/` must not be named `test_*.py`.
- Tests that need real model weights must be marked `@pytest.mark.model` so they are skipped by default.
- Share one `FashionCLIPModel` instance across classifiers. Loading it is the slow part.

## Housekeeping

- `ai/classifier2/` and `ai/recommendation/` currently contain only `__pycache__` folders (the source files are gone). They are safe to delete, and `__pycache__`, `.pytest_cache` and `uploads/` should be in `.gitignore`.
- `tests/images/` holds two images (`nike_dress.jpg`, `red_polo.jpg`) that no current test references.
- `evaluate_classifier.py` exists both in `python/` and in `ai/classifier/`. Keep one.