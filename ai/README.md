# Vestir AI Models

## Overview

This directory contains the AI models used by Vestir to identify clothing categories and predict the main color of clothing items from images.

Vestir uses FashionCLIP to perform image-text similarity comparisons. The models are intended to help users add clothing items to their digital wardrobe by providing initial attribute predictions.

## Current Features

### 1. Clothing Category Classification

The clothing classifier predicts one of four categories:

- `top` — T-shirts, blouses, sweaters, and hoodies
- `bottom` — Pants, jeans, skirts, and shorts
- `dress` — Dresses, jumpsuits, and rompers
- `outerwear` — Jackets, coats, blazers, and similar items

The classifier returns the predicted category, a confidence score, and scores for all four categories.

### 2. Main Color Detection

The color detector predicts the main color of a clothing item.

The current color vocabulary includes:

- Black, white, gray, and charcoal
- Brown, beige, khaki, and cream
- Navy blue, blue, and light blue
- Green and olive green
- Red, pink, purple, yellow, orange, and burgundy

The detector returns a predicted main color and the five highest-ranked color matches.

Color detection provides a general color suggestion rather than detailed shade identification. Predictions may not always be correct.

### 3. Shared FashionCLIP Model

Both detectors can use the same `FashionCLIPModel` instance instead of loading separate copies of FashionCLIP.

This reduces redundant model loading when both category and color predictions are needed for the same image-processing workflow.

## Project Files

| File | Purpose |
|---|---|
| `fashionclip_model.py` | Loads FashionCLIP and provides shared image and text encoding |
| `clothing_classifier.py` | Predicts clothing categories |
| `fashionclip_color_detector.py` | Predicts main clothing colors |
| `test_classifier.py` | Tests category classification |
| `test_fashionclip_color.py` | Tests color detection |
| `test_combined_detectors.py` | Tests both detectors using one shared model instance |
| `requirements.txt` | Lists Python dependencies |

## Requirements

- Python 3.10 or later
- The dependencies listed in `requirements.txt`
- Internet access when downloading the FashionCLIP model for the first time

Install the dependencies from the project root:

```bash
pip install -r ai/requirements.txt
```

## Running the Tests

Run the category classifier test:

```bash
python ai/test_classifier.py
```

Run the color detector test:

```bash
python ai/test_fashionclip_color.py
```

Run both detectors using a shared model:

```bash
python ai/test_combined_detectors.py
```

The combined test uses `ai/test_image.jpeg` as its test image. Make sure this image exists before running the test.

## Integration Workflow

The intended workflow for adding a clothing item is:

1. The user uploads a clothing image.
2. FashionCLIP predicts the clothing category and main color.
3. The application presents the predictions to the user.
4. The user can review and correct the predicted attributes.
5. The backend saves the final clothing attributes to the database.

The category classifier and color detector are intended to run during clothing image upload, not every time an outfit recommendation is requested.

The user-correction interface, database persistence, and backend integration are application responsibilities and are not implemented by these model classes.

## Limitations

- Category and color predictions can be incorrect.
- Color detection identifies a general main color and may not distinguish subtle shades reliably.
- Prediction scores are relative to the candidate labels and should not be interpreted as calibrated probabilities.
- The current implementation does not provide a complete outfit recommendation engine.

## Future Development

Potential future improvements include:

- Integrating the detectors with the Vestir backend and frontend.
- Allowing users to correct and save predicted clothing attributes.
- Evaluating model performance on a larger and more varied set of clothing images.
- Improving color detection if evaluation shows that greater detail is necessary.
- Integrating the saved clothing attributes with the outfit recommendation engine.

## Model

Vestir uses [Marqo FashionCLIP](https://huggingface.co/Marqo/marqo-fashionCLIP), accessed through the `open_clip` library.
