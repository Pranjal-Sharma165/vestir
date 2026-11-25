
from pathlib import Path

import pytest
from PIL import Image

from ai.classifier.pipeline import ClothingPipeline


class FakeModel:
    
    logit_scale = 100.0

    def encode_text(self, prompts):
        return None

    def encode_image(self, image):
        return "fake_features"


class FakePredictor:
    def __init__(self, result):
        self.result = result

    def predict_from_features(self, features):
        return self.result


class FakeVerifier:
    def __init__(self, result):
        self.result = result

    def check_features(self, features):
        return self.result


def make_pipeline(verify=True):
    pipeline = ClothingPipeline(
        shared_model=FakeModel(),
        verify=verify,
    )

    pipeline.category = FakePredictor({
        "label": "top",
        "confidence": 0.95,
    })

    pipeline.subs = {
        "top": FakePredictor({
            "label": "t-shirt",
            "confidence": 0.90,
        })
    }

    pipeline.color = FakePredictor({
        "color": "black",
        "confidence": 0.88,
    })

    pipeline.formality = FakePredictor({
        "formality": "casual",
        "confidence": 0.92,
    })

    if verify:
        pipeline.verifier = FakeVerifier({
            "is_clothing": True,
            "clothing_probability": 0.98,
        })

    return pipeline


def test_invalid_image_returns_error(tmp_path):
    pipeline = make_pipeline()

    result = pipeline.analyze(tmp_path / "missing.jpg")

    assert result == {
        "ok": False,
        "error": "Invalid image",
    }


def test_valid_image_returns_classification(tmp_path):
    image_path = tmp_path / "shirt.png"
    Image.new("RGB", (100, 100), color="black").save(image_path)

    pipeline = make_pipeline()

    result = pipeline.analyze(image_path)

    assert result["ok"] is True
    assert result["category"]["label"] == "top"
    assert result["sub_category"]["label"] == "t-shirt"
    assert result["color"]["color"] == "black"
    assert result["formality"]["formality"] == "casual"
    assert result["clothing_probability"] == 0.98


def test_non_clothing_image_is_rejected(tmp_path):
    image_path = tmp_path / "not_clothing.png"
    Image.new("RGB", (100, 100), color="blue").save(image_path)

    pipeline = make_pipeline()

    pipeline.verifier = FakeVerifier({
        "is_clothing": False,
        "clothing_probability": 0.12,
    })

    result = pipeline.analyze(image_path)

    assert result == {
        "ok": False,
        "error": "Invalid image",
    }


def test_pipeline_without_verification(tmp_path):
    image_path = tmp_path / "shirt.png"
    Image.new("RGB", (100, 100), color="black").save(image_path)

    pipeline = make_pipeline(verify=False)

    result = pipeline.analyze(image_path)

    assert result["ok"] is True
    assert result["clothing_probability"] is None
