# Vestir AI Models

## Overview

This directory contains the AI models used by Vestir to analyze clothing images and predict clothing attributes.

Vestir uses FashionCLIP to predict clothing category, main color, and formality. These predictions help users add clothing items to their digital wardrobe.

The detectors are designed to share a single FashionCLIP model instance when multiple predictions are needed for the same image-processing workflow.

## Current Features

### 1. Clothing Category Classification

The clothing classifier predicts one of four categories:

- `top` — T-shirts, blouses, sweaters, and hoodies
- `bottom` — Pants, jeans, skirts, and shorts
- `dress` — Dresses, jumpsuits, and rompers
- `outerwear` — Jackets, coats, blazers, and similar garments

The classifier returns the predicted category, a confidence score, and scores for all four categories.

### 2. Main Color Detection

The color detector predicts the main color of a clothing item.

The current color vocabulary includes:

- Black, white, gray, and charcoal
- Brown, beige, khaki, and cream
- Navy blue, blue, and light blue
- Green and olive green
- Red, pink, purple, yellow, orange, and burgundy

The detector returns the predicted main color and the five highest-ranked color matches.

Color detection provides a general color suggestion rather than detailed shade identification. Predictions may not always be correct.

### 3. Formality Detection

The formality detector predicts one of four clothing styles:

- `casual` — Everyday, relaxed clothing
- `smart-casual` — Clothing that combines casual comfort with a more polished appearance
- `formal` — Business, tailored, or occasion clothing
- `sporty` — Athletic clothing and sportswear

The detector compares clothing image features with text descriptions of the four styles and returns the predicted formality along with ranked scores.

Formality detection is experimental. A garment's formality can depend on its design and context, so predictions should be treated as suggestions rather than definitive labels.

## Shared FashionCLIP Model

The `FashionCLIPModel` class loads FashionCLIP and provides methods for encoding images and text descriptions.

The category classifier, color detector, and formality detector can reuse the same model instance. This avoids loading separate copies of FashionCLIP when all three predictions are required.

Each detector prepares its own text features for its specific task.

## Project Structure

| File | Purpose |
|---|---|
| `fashionclip_model.py` | Loads FashionCLIP and provides shared image and text encoding |
| `clothing_classifier.py` | Predicts clothing categories |
| `fashionclip_color_detector.py` | Predicts main clothing colors |
| `formality_detector.py` | Predicts clothing formality |
| `tester.py` | Tests category, color, and formality detection together |
| `test_fashionclip_color.py` | Tests color detection independently |
| `requirements.txt` | Lists Python dependencies |

## Requirements

- Python 3.10 or later
- Dependencies listed in `requirements.txt`
- Internet access when downloading the FashionCLIP model for the first time

Install the dependencies from the project root:

```bash
pip install -r ai/requirements.txt
```

## Running the Tests

Run all three detectors using a shared FashionCLIP model:

```bash
python ai/tester.py
```

The test script prints the predicted category, main color, and formality for the configured test image.

Make sure the image path configured in `tester.py` points to an existing image before running the test.

To test color detection independently:

```bash
python ai/test_fashionclip_color.py
```

The first model run may take longer because FashionCLIP needs to be downloaded and initialized.

## Integration Workflow

The intended workflow for adding a clothing item is:

1. The user uploads a clothing image.
2. FashionCLIP predicts the category, main color, and formality.
3. The application displays the predictions to the user.
4. The user can review and correct the predicted attributes.
5. The backend saves the final clothing attributes to the database.

The detectors are intended to run when a clothing image is uploaded, rather than every time an outfit recommendation is requested.

The user's final saved attributes should be used by the recommendation engine. AI predictions should not silently overwrite user corrections.

The user interface, database persistence, and backend integration are application responsibilities and are not implemented by the detector classes themselves.

## Weather and Outfit Recommendations

Weather conditions are separate from clothing image attributes.

The planned backend will obtain temperature and rain conditions through a weather API. During development, these values can be hardcoded for testing.

The planned outfit recommendation engine will use saved clothing attributes together with information such as:

- Available clothing items
- Clothing category and color
- Formality and occasion requirements
- Current temperature and rain conditions
- User preferences, where supported

The recommendation engine is a separate component and is not implemented by the image detectors.

## Limitations

- Category, color, and formality predictions may be incorrect.
- Color detection identifies a general main color and may not distinguish subtle shades reliably.
- Formality is subjective and can depend on the garment's design and intended use.
- Prediction scores are relative to the candidate labels and should not be interpreted as calibrated probabilities.
- The current detectors do not implement a complete outfit recommendation system.

## Future Development

Potential improvements include:

- Integrating the detectors with the Vestir backend and frontend
- Allowing users to review and correct predicted attributes
- Saving user-approved clothing attributes in the database
- Evaluating prediction quality on a larger and more varied clothing dataset
- Integrating saved wardrobe attributes with the outfit recommendation engine
- Improving detection quality if evaluation shows that further work is needed

## Model and Library

Vestir uses [Marqo FashionCLIP](https://huggingface.co/Marqo/marqo-fashionCLIP) through the [`open_clip`](https://github.com/mlfoundations/open_clip) library.
