import argparse
from pathlib import Path

import pandas as pd
from sklearn.metrics import classification_report, confusion_matrix

from clothing_classifier import ClothingClassifier


LABEL_NAMES = ["top", "bottom", "dress", "outerwear"]


def main():
    parser = argparse.ArgumentParser(
        description="Evaluate the Vestir clothing classifier."
    )
    parser.add_argument(
        "--csv",
        required=True,
        help="Path to the evaluation CSV.",
    )
    parser.add_argument(
        "--images-dir",
        required=True,
        help="Directory containing the evaluation images.",
    )
    parser.add_argument(
        "--output",
        default="evaluation_results.csv",
        help="Path for the detailed results CSV.",
    )
    args = parser.parse_args()

    csv_path = Path(args.csv)
    images_dir = Path(args.images_dir)

    df = pd.read_csv(csv_path)

    required_columns = {"image", "vestir_label"}
    if not required_columns.issubset(df.columns):
        raise ValueError(
            f"CSV must contain these columns: {required_columns}"
        )

    print(f"Images to evaluate: {len(df)}")

    # Load the model only once.
    classifier = ClothingClassifier()

    predictions = []
    confidences = []

    for i, row in df.iterrows():
        image_name = str(row["image"])
        image_path = images_dir / image_name

        # Support IDs stored without a file extension.
        if not image_path.is_file() and not image_path.suffix:
            image_path = image_path.with_suffix(".jpg")

        result = classifier.predict(image_path)
        predictions.append(result["category"])
        confidences.append(result["confidence"])

        if (i + 1) % 50 == 0:
            print(f"Processed {i + 1}/{len(df)}")

    df["prediction"] = predictions
    df["confidence"] = confidences

    accuracy = (df["vestir_label"] == df["prediction"]).mean()

    print(f"\nAccuracy: {accuracy:.3f} ({accuracy * 100:.2f}%)")

    print("\nClassification Report:")
    print(
        classification_report(
            df["vestir_label"],
            df["prediction"],
            labels=LABEL_NAMES,
            digits=3,
            zero_division=0,
        )
    )

    print("\nConfusion Matrix:")
    print(
        confusion_matrix(
            df["vestir_label"],
            df["prediction"],
            labels=LABEL_NAMES,
        )
    )

    output_path = Path(args.output)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(output_path, index=False)

    print(f"\nDetailed results saved to: {output_path}")


if __name__ == "__main__":
    main()