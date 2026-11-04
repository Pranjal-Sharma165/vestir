import argparse
from pathlib import Path

import pandas as pd
from sklearn.metrics import classification_report, confusion_matrix

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline


CATEGORY = ["top", "bottom", "dress", "outerwear"]


def report(title, y_true, y_pred, labels):
    """Print accuracy, classification report and confusion matrix."""
    accuracy = (y_true == y_pred).mean()
    print(f"\n=== {title} ===")
    print(f"Accuracy: {accuracy:.3f} ({accuracy * 100:.2f}%)")

    print("\nClassification Report:")
    print(
        classification_report(
            y_true,
            y_pred,
            labels=labels,
            digits=3,
            zero_division=0,
        )
    )

    print("Confusion Matrix (rows = true, columns = predicted):")
    print(pd.DataFrame(
        confusion_matrix(y_true, y_pred, labels=labels),
        index=labels,
        columns=labels,
    ))


def main():
    parser = argparse.ArgumentParser(
        description="Evaluate the Vestir clothing classifier."
    )
    parser.add_argument(
        "--csv",
        default="ai/evaluation.csv",
        help="Path to the evaluation CSV.",
    )
    parser.add_argument(
        "--images-dir",
        default="ai",
        help="Directory containing the evaluation images.",
    )
    parser.add_argument(
        "--output",
        default="ai/category_evaluation_results.csv",
        help="Path for the detailed results CSV.",
    )
    args = parser.parse_args()

    csv_path = Path(args.csv)
    images_dir = Path(args.images_dir)

    df = pd.read_csv(csv_path)

    required_columns = ["image", "category"]
    if not set(required_columns).issubset(df.columns):
        raise ValueError(
            f"CSV must contain these columns: {required_columns}"
        )

    # "sub_category" is optional. If present, it is evaluated too.
    has_sub = "sub_category" in df.columns
    columns = required_columns + (["sub_category"] if has_sub else [])
    df = df[columns].copy()

    print(f"Images to evaluate: {len(df)}")

    # Load the model only once and share it with the pipeline.
    shared_model = FashionCLIPModel()
    pipeline = ClothingPipeline(shared_model)

    predictions = []
    confidences = []
    sub_predictions = []
    sub_confidences = []

    for i, row in enumerate(df.itertuples(index=False), start=1):
        image_path = images_dir / str(row.image)

        # Support IDs stored without a file extension.
        if not image_path.is_file() and not image_path.suffix:
            image_path = image_path.with_suffix(".jpg")

        result = pipeline.predict(image_path)

        # Results use "label" / "confidence" keys.
        predictions.append(result["category"]["label"])
        confidences.append(result["category"]["confidence"])
        sub_predictions.append(result["sub_category"]["label"])
        sub_confidences.append(result["sub_category"]["confidence"])

        if i % 50 == 0:
            print(f"Processed {i}/{len(df)}")

    df["prediction"] = predictions
    df["confidence"] = confidences
    df["sub_prediction"] = sub_predictions
    df["sub_confidence"] = sub_confidences

    report("Category", df["category"], df["prediction"], CATEGORY)

    if has_sub:
        # End-to-end: a wrong stage-1 category also makes stage 2 wrong.
        sub_labels = sorted(set(df["sub_category"]) | set(df["sub_prediction"]))
        report(
            "Sub-category (end-to-end)",
            df["sub_category"],
            df["sub_prediction"],
            sub_labels,
        )

        # Isolates stage 2 by looking only at images stage 1 got right.
        correct = df[df["category"] == df["prediction"]]
        if len(correct):
            acc = (correct["sub_category"] == correct["sub_prediction"]).mean()
            print(
                f"\nSub-category accuracy when category is correct: "
                f"{acc:.3f} ({len(correct)} images)"
            )

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(output_path, index=False)

    print(f"\nDetailed results saved to: {output_path}")


if __name__ == "__main__":
    main()