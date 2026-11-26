from pathlib import Path

import pandas as pd
import pytest

from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.formality_detector import FormalityDetector
from ai.classifier.pipeline import ClothingPipeline

pytestmark = pytest.mark.model

DATA = Path(__file__).parent / "data"
MIN_CATEGORY_ACC = 0.85
MIN_SUBCATEGORY_ACC = 0.60
MIN_COLOR_ACC = 0.60
MIN_ACCEPT_RATE = 0.90


@pytest.fixture(scope="module")
def pipeline():
    return ClothingPipeline(FashionCLIPModel())


@pytest.fixture(scope="module")
def manifest():
    df = pd.read_csv(DATA / "samples.csv", dtype=str, keep_default_na=False)
    df["expected_ok"] = df["expected_ok"].str.lower() == "true"
    return df


@pytest.fixture(scope="module")
def results(pipeline, manifest):
    return {
        row.image: pipeline.analyze(DATA / "samples" / row.image)
        for row in manifest.itertuples()
    }


def _accuracy(manifest, results, column, getter):
    rows = manifest[manifest.expected_ok & (manifest[column] != "")]
    misses = []
    for r in rows.itertuples():
        res = results[r.image]
        got = getter(res) if res["ok"] else "<rejected>"
        if got != getattr(r, column):
            misses.append(f"{r.image}: expected {getattr(r, column)!r}, got {got!r}")
    return 1 - len(misses) / len(rows), misses


def test_manifest_images_exist(manifest):
    missing = [i for i in manifest.image if not (DATA / "samples" / i).is_file()]
    assert not missing


def test_clothing_photos_pass_verification(manifest, results):
    clothes = manifest[manifest.expected_ok]
    rejected = [i for i in clothes.image if not results[i]["ok"]]
    rate = 1 - len(rejected) / len(clothes)
    assert rate >= MIN_ACCEPT_RATE, f"wrongly rejected: {rejected}"


def test_non_clothing_photos_are_rejected(manifest, results):
    bad = manifest[~manifest.expected_ok]
    accepted = [i for i in bad.image if results[i]["ok"]]
    assert not accepted, f"non-clothing accepted: {accepted}"


def test_category_accuracy(manifest, results):
    acc, misses = _accuracy(manifest, results, "category", lambda r: r["category"]["label"])
    assert acc >= MIN_CATEGORY_ACC, f"{acc:.2f}\n" + "\n".join(misses)


def test_subcategory_accuracy(manifest, results):
    acc, misses = _accuracy(
        manifest, results, "sub_category", lambda r: r["sub_category"]["label"]
    )
    assert acc >= MIN_SUBCATEGORY_ACC, f"{acc:.2f}\n" + "\n".join(misses)


def test_color_accuracy(manifest, results):
    acc, misses = _accuracy(manifest, results, "color", lambda r: r["color"]["color"])
    assert acc >= MIN_COLOR_ACC, f"{acc:.2f}\n" + "\n".join(misses)


def test_outputs_are_well_formed(manifest, results):
    for image, res in results.items():
        if not res["ok"]:
            continue
        for key in ("category", "sub_category"):
            ranked = [r["score"] for r in res[key]["ranked"]]
            assert ranked == sorted(ranked, reverse=True), image
            assert 0.0 <= res[key]["confidence"] <= 1.0, image
        assert res["color"]["color"] in FashionCLIPColorDetector.COLORS, image
        assert res["formality"]["formality"] in FormalityDetector.FORMALITIES, image
        assert 0.0 <= res["clothing_probability"] <= 1.0, image


def test_subcategory_belongs_to_predicted_category(results):
    from ai.classifier.subcategory_classifier import (
        BottomClassifier, DressClassifier, OuterwearClassifier, TopClassifier,
    )
    allowed = {
        "top": TopClassifier.LABEL_NAMES,
        "bottom": BottomClassifier.LABEL_NAMES,
        "dress": DressClassifier.LABEL_NAMES,
        "outerwear": OuterwearClassifier.LABEL_NAMES,
    }
    for image, res in results.items():
        if res["ok"]:
            assert res["sub_category"]["label"] in allowed[res["category"]["label"]], image


def test_analysis_is_deterministic(pipeline):
    path = DATA / "samples" / "img_1973.jpg"
    assert pipeline.analyze(path) == pipeline.analyze(path)


def test_original_and_downscaled_agree_on_category(pipeline, monkeypatch):
    monkeypatch.setattr(pipeline, "verifier", None)
    path = DATA / "samples" / "my_jeans.jpg"
    assert (
        pipeline.analyze(path)["category"]["label"]
        == pipeline.predict(path)["category"]["label"]
    )