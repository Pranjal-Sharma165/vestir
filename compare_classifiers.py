"""
Compare two classifier packages on your labeled images.

python compare_classifiers.py --labels uploads_correct_classifications.csv --images uploads --a ai.classifier --b ai.classifier2
"""
import argparse
import csv
import importlib
from collections import Counter
from pathlib import Path

FIELDS = ["category", "sub_category", "color", "formality"]


def read_labels(path):
    with open(path, newline="", encoding="utf-8-sig") as f:
        sample = f.read(2048)
        f.seek(0)
        delimiter = "\t" if "\t" in sample.splitlines()[0] else ","
        rows = []
        for row in csv.DictReader(f, delimiter=delimiter):
            rows.append({k.strip(): (v or "").strip() for k, v in row.items() if k})
    return rows


class Runner:
    """Loads one package and exposes a uniform predict(path) -> dict."""

    def __init__(self, package):
        self.package = package
        mod = lambda name: importlib.import_module(f"{package}.{name}")

        model = mod("fashionclip_model").FashionCLIPModel()
        self.pipeline = mod("pipeline").ClothingPipeline(model)
        self.is_new = hasattr(self.pipeline, "predict_one")

        if not self.is_new:
            self.color = mod("fashionclip_color_detector").FashionCLIPColorDetector(model)
            self.formality = mod("formality_detector").FormalityDetector(model)

    def predict(self, path):
        if self.is_new:
            r = self.pipeline.predict_one(path)
            return {
                "category": r["category_name"],
                "sub_category": r["sub_category"],
                "color": r["primary_color"],
                "formality": r["formality"],
                # confidence
                "category_conf": r["confidence"]["category"],
                "sub_category_conf": r["confidence"]["sub_category"],
                "color_conf": r["confidence"]["color"],
                "formality_conf": r["confidence"]["formality"],
                "low_confidence_formality": r.get("low_confidence_formality", False),
            }

        # old layout
        r = self.pipeline.predict(path)
        color_r = self.color.predict(path)
        form_r = self.formality.predict(path)
        return {
            "category": r["category"]["label"],
            "sub_category": r["sub_category"]["label"],
            "color": color_r["color"],
            "formality": form_r["formality"],
            "category_conf": r["category"].get("confidence"),
            "sub_category_conf": r["sub_category"].get("confidence"),
            "color_conf": color_r.get("confidence"),
            "formality_conf": form_r.get("confidence"),
            "low_confidence_formality": False,
        }


def evaluate(runner, rows, image_dir):
    results = []
    for row in rows:
        path = image_dir / row["image"]
        try:
            pred = runner.predict(path)
        except Exception as e:  # keep going so one bad file doesn't stop the run
            pred = {f: f"ERROR: {type(e).__name__}" for f in FIELDS}
            for f in FIELDS:
                pred[f"{f}_conf"] = None
            pred["low_confidence_formality"] = False
        results.append((row, pred))
    return results


def summarize(name, results):
    print(f"\n{'=' * 60}\n{name}\n{'=' * 60}")
    for field in FIELDS:
        correct = sum(1 for row, pred in results if pred[field] == row[field])
        print(f"{field:13}: {correct}/{len(results)} correct")

        # confidence stats
        conf_key = f"{field}_conf"
        correct_confs = [
            pred[conf_key]
            for row, pred in results
            if pred[field] == row[field] and pred.get(conf_key) is not None
        ]
        wrong_confs = [
            pred[conf_key]
            for row, pred in results
            if pred[field] != row[field] and pred.get(conf_key) is not None
        ]

        if correct_confs:
            avg_c = sum(correct_confs) / len(correct_confs)
            print(f"             avg conf (correct): {avg_c:.3f}")
        if wrong_confs:
            avg_w = sum(wrong_confs) / len(wrong_confs)
            print(f"             avg conf (wrong)  : {avg_w:.3f}")

    for field in FIELDS:
        wrong = Counter(
            (row[field], pred[field])
            for row, pred in results
            if pred[field] != row[field]
        )
        if wrong:
            print(f"\nConfusions for {field} (expected -> predicted):")
            for (exp, got), n in wrong.most_common():
                print(f"  {exp} -> {got}  x{n}")


def side_by_side(name_a, res_a, name_b, res_b):
    print(f"\n{'=' * 60}\nPER IMAGE (expected | A | B)\n{'=' * 60}")
    print(f"A = {name_a}   B = {name_b}")
    for (row, pa), (_, pb) in zip(res_a, res_b):
        print(f"\n{row['image']}")
        for field in FIELDS:
            exp, a, b = row[field], pa[field], pb[field]
            mark_a = "ok " if a == exp else "BAD"
            mark_b = "ok " if b == exp else "BAD"

            # confidence strings
            conf_a = pa.get(f"{field}_conf")
            conf_b = pb.get(f"{field}_conf")
            conf_a_str = f" ({conf_a:.2f})" if isinstance(conf_a, float) else ""
            conf_b_str = f" ({conf_b:.2f})" if isinstance(conf_b, float) else ""

            print(
                f"  {field:13} expected={exp:14} "
                f"A[{mark_a}]={a:14}{conf_a_str}  "
                f"B[{mark_b}]={b:14}{conf_b_str}"
            )

        # formality low-confidence flag (new layout only)
        if pa.get("low_confidence_formality") or pb.get("low_confidence_formality"):
            print(
                f"  {'formality flag':13} "
                f"A={pa.get('low_confidence_formality')}  "
                f"B={pb.get('low_confidence_formality')}"
            )


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--labels", required=True)
    ap.add_argument("--images", required=True, help="folder containing the images")
    ap.add_argument("--a", default="ai.classifier")
    ap.add_argument("--b", default="ai.classifier2")
    args = ap.parse_args()

    rows = read_labels(args.labels)
    image_dir = Path(args.images)
    print(f"{len(rows)} labeled images")

    res_a = evaluate(Runner(args.a), rows, image_dir)
    res_b = evaluate(Runner(args.b), rows, image_dir)

    summarize(f"A: {args.a}", res_a)
    summarize(f"B: {args.b}", res_b)
    side_by_side(args.a, res_a, args.b, res_b)


if __name__ == "__main__":
    main()