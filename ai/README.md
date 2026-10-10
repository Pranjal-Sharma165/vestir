# Vestir AI — Clothing Classifier

## Overview

Vestir uses FashionCLIP to classify clothing images into four categories:

- **Top:** T-shirts, blouses, sweaters, and hoodies.
- **Bottom:** Pants, jeans, skirts, and shorts.
- **Dress:** Dresses, jumpsuits, and rompers.
- **Outerwear:** Jackets, coats, blazers, and cardigans.

The classifier uses a pretrained model with zero-shot classification. It compares an input clothing image with text descriptions of the four categories and predicts the category with the highest similarity score.

## Model

- **Model:** Marqo FashionCLIP
- **Hugging Face model ID:** `Marqo/marqo-fashionCLIP`
- **Library:** OpenCLIP
- **Framework:** PyTorch

The model is loaded when the `ClothingClassifier` is initialized. The same instance can then be reused for multiple predictions.

## Setup

Run these commands from the root of the Vestir repository.

Install the required dependencies:

```bash
pip install -r ai/requirements.txt
```

## Run a Test

The test script loads the model and classifies a sample image.

```bash
python ai/test_classifier.py
```

The test script expects a local test image at `ai/test_image.jpeg`. This image is for local testing and is not included in the repository.

## Evaluate the Model

The evaluation script compares predictions against labeled images and reports accuracy, precision, recall, F1-score, and a confusion matrix.

Provide paths to an evaluation CSV and its image directory:

```bash
python ai/evaluate_classifier.py --csv "PATH_TO_EVALUATION_CSV" --images-dir "PATH_TO_IMAGES" --output "evaluation_results.csv"
```

The CSV must contain these columns:

- `image`: Image filename or image ID without its extension.
- `vestir_label`: Correct category (`top`, `bottom`, `dress`, or `outerwear`).

The evaluation output CSV contains the original data, predicted category, and confidence score.

## Evaluation Results

On an evaluation dataset of 800 images, with 200 images per category, the model achieved:

- **Accuracy:** 91% (728 out of 800 images)
- **Top recall:** 94.5%
- **Bottom recall:** 100%
- **Dress recall:** 80%
- **Outerwear recall:** 89.5%

These results apply to the specific evaluation dataset and are not a guarantee of performance on all wardrobe images.

The most common error was classifying dresses as tops, with 33 such misclassifications.

## Limitations

- Predictions can be incorrect, particularly for clothing items that resemble multiple categories.
- The returned scores are relative model scores and should not be interpreted as calibrated probabilities.
- Color detection and outfit recommendation are not implemented by this classifier.
- Evaluation results depend on the dataset and its labels.

## Project Files

- `clothing_classifier.py`: Classifier implementation.
- `evaluate_classifier.py`: Evaluation script.
- `test_classifier.py`: Local test script.
- `requirements.txt`: Python dependencies.
