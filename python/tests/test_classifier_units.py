import pytest
import torch
from PIL import Image

from ai.classifier.base_classifier import BaseZeroShotClassifier
from ai.classifier.clothing_classifier import ClothingClassifier
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.formality_detector import FormalityDetector
from ai.classifier.image_verifier import ImageVerifier
from ai.classifier.pipeline import ClothingPipeline
from ai.classifier.subcategory_classifier import (
    BottomClassifier,
    DressClassifier,
    OuterwearClassifier,
    TopClassifier,
)


class EyeModel:
    logit_scale = 100.0

    def __init__(self):
        self.encoded_sizes = []

    def encode_text(self, prompts):
        return torch.eye(len(prompts))

    def encode_image(self, image):
        if isinstance(image, Image.Image):
            self.encoded_sizes.append(image.size)
        return "fake_features"


def one_hot(n, i):
    v = torch.zeros(1, n)
    v[0, i] = 1.0
    return v


SUB_CLASSES = [TopClassifier, BottomClassifier, DressClassifier, OuterwearClassifier]
ALL_CLASSIFIERS = [ClothingClassifier] + SUB_CLASSES

@pytest.mark.parametrize("cls", ALL_CLASSIFIERS)
def test_labels_and_names_have_same_length_and_no_duplicates(cls):
    assert len(cls.LABELS) == len(cls.LABEL_NAMES) > 0
    assert len(set(cls.LABEL_NAMES)) == len(cls.LABEL_NAMES)


def test_mismatched_labels_raise():
    class Broken(BaseZeroShotClassifier):
        LABEL_NAMES = ["a", "b"]
        LABELS = ["a photo of a"]

    with pytest.raises(ValueError, match="LABELS and LABEL_NAMES"):
        Broken(EyeModel())


def test_every_category_has_a_subcategory_classifier():
    pipeline = ClothingPipeline(EyeModel(), verify=False)
    assert set(pipeline.subs) == set(ClothingClassifier.LABEL_NAMES)


def test_color_and_formality_prompts_match_labels():
    model = EyeModel()
    assert FashionCLIPColorDetector(model).text_features.shape[0] == len(
        FashionCLIPColorDetector.COLORS
    )
    assert len(FormalityDetector.PROMPTS) == len(FormalityDetector.FORMALITIES)

@pytest.mark.parametrize("cls", ALL_CLASSIFIERS)
def test_each_label_is_reachable(cls):
    clf = cls(EyeModel())
    n = len(cls.LABEL_NAMES)
    for i, name in enumerate(cls.LABEL_NAMES):
        out = clf.predict_from_features(one_hot(n, i))
        assert out["label"] == name


def test_ranked_output_is_sorted_and_sums_to_one():
    clf = TopClassifier(EyeModel())
    out = clf.predict_from_features(one_hot(len(TopClassifier.LABEL_NAMES), 2))
    scores = [r["score"] for r in out["ranked"]]
    assert scores == sorted(scores, reverse=True)
    assert len(scores) == len(TopClassifier.LABEL_NAMES)
    assert sum(scores) == pytest.approx(1.0, abs=1e-2)
    assert out["confidence"] == scores[0]
    assert 0.0 <= out["confidence"] <= 1.0


def test_color_detector_picks_matching_color_and_returns_top5():
    det = FashionCLIPColorDetector(EyeModel())
    i = det.COLORS.index("burgundy")
    out = det.predict_from_features(one_hot(len(det.COLORS), i))
    assert out["color"] == "burgundy"
    assert len(out["ranked_colors"]) == 5


def test_formality_detector_picks_matching_formality():
    det = FormalityDetector(EyeModel())
    for i, name in enumerate(det.FORMALITIES):
        out = det.predict_from_features(one_hot(len(det.FORMALITIES), i))
        assert out["formality"] == name


def test_predict_missing_file_raises(tmp_path):
    clf = ClothingClassifier(EyeModel())
    with pytest.raises(FileNotFoundError):
        clf.predict(tmp_path / "nope.jpg")

def _verifier(**kw):
    return ImageVerifier(EyeModel(), **kw)


def _prompt_index(verifier, group, offset=0):
    return verifier.slices[group].start + offset


def test_verifier_accepts_clothing_prompt():
    v = _verifier()
    n = v.text_features.shape[0]
    out = v.check_features(one_hot(n, _prompt_index(v, "clothing")))
    assert out["is_clothing"] is True
    assert out["clothing_probability"] > 0.9


@pytest.mark.parametrize("group", ["accessories", "other"])
def test_verifier_rejects_non_clothing_groups(group):
    v = _verifier()
    n = v.text_features.shape[0]
    out = v.check_features(one_hot(n, _prompt_index(v, group)))
    assert out["is_clothing"] is False
    assert out["clothing_probability"] < 0.1


def test_verifier_uses_max_not_sum_per_group():
    v = _verifier()
    n = v.text_features.shape[0]
    uniform = torch.ones(1, n) / n**0.5
    out = v.check_features(uniform)
    assert out["clothing_probability"] == pytest.approx(1 / 3, abs=0.01)


def test_verifier_threshold_is_configurable():
    n = _verifier().text_features.shape[0]
    strict = _verifier(min_clothing_prob=0.999999)
    lenient = _verifier(min_clothing_prob=0.0)
    uniform = torch.ones(1, n) / n**0.5
    assert strict.check_features(uniform)["is_clothing"] is False
    assert lenient.check_features(uniform)["is_clothing"] is True

class Recorder:
    def __init__(self, result):
        self.result = result
        self.calls = 0

    def predict_from_features(self, features):
        self.calls += 1
        return self.result


class Verifier:
    def __init__(self, ok=True):
        self.ok = ok

    def check_features(self, features):
        return {"is_clothing": self.ok, "clothing_probability": 0.9 if self.ok else 0.1}


def _pipeline(category, model=None, verify=True):
    p = ClothingPipeline(model or EyeModel(), verify=verify)
    p.category = Recorder({"label": category, "confidence": 0.9})
    p.subs = {c: Recorder({"label": f"{c}-sub", "confidence": 0.8})
              for c in ClothingClassifier.LABEL_NAMES}
    p.color = Recorder({"color": "black", "confidence": 0.7})
    p.formality = Recorder({"formality": "casual", "confidence": 0.6})
    if verify:
        p.verifier = Verifier(True)
    return p


@pytest.fixture
def img(tmp_path):
    path = tmp_path / "x.png"
    Image.new("RGB", (64, 64), "black").save(path)
    return path


@pytest.mark.parametrize("category", ClothingClassifier.LABEL_NAMES)
def test_analyze_routes_to_matching_subclassifier_only(category, img):
    p = _pipeline(category)
    result = p.analyze(img)
    assert result["ok"] is True
    assert result["sub_category"]["label"] == f"{category}-sub"
    for name, rec in p.subs.items():
        assert rec.calls == (1 if name == category else 0)


@pytest.mark.parametrize("category", ClothingClassifier.LABEL_NAMES)
def test_predict_routes_to_matching_subclassifier(category, img):
    p = _pipeline(category)
    out = p.predict(img)
    assert out["category"]["label"] == category
    assert out["sub_category"]["label"] == f"{category}-sub"


def test_corrupt_file_is_invalid(tmp_path):
    bad = tmp_path / "bad.jpg"
    bad.write_text("this is not an image")
    assert _pipeline("top").analyze(bad) == {"ok": False, "error": "Invalid image"}


def test_empty_file_is_invalid(tmp_path):
    empty = tmp_path / "empty.jpg"
    empty.write_bytes(b"")
    assert _pipeline("top").analyze(empty)["ok"] is False


def test_directory_path_is_invalid(tmp_path):
    assert _pipeline("top").analyze(tmp_path)["ok"] is False


def test_rejected_image_skips_all_classifiers(img):
    p = _pipeline("top")
    p.verifier = Verifier(False)
    assert p.analyze(img) == {"ok": False, "error": "Invalid image"}
    assert p.category.calls == 0
    assert p.color.calls == 0
    assert p.formality.calls == 0


def test_large_image_is_downscaled_before_encoding(tmp_path):
    big = tmp_path / "big.png"
    Image.new("RGB", (3000, 2000), "white").save(big)
    model = EyeModel()
    _pipeline("top", model=model).analyze(big)
    w, h = model.encoded_sizes[-1]
    assert max(w, h) <= 1024
    assert w / h == pytest.approx(1.5, abs=0.01)


def test_small_image_is_not_upscaled(img):
    model = EyeModel()
    _pipeline("top", model=model).analyze(img)
    assert model.encoded_sizes[-1] == (64, 64)


@pytest.mark.parametrize("mode", ["L", "RGBA", "P"])
def test_non_rgb_modes_are_accepted(tmp_path, mode):
    path = tmp_path / f"{mode}.png"
    Image.new(mode, (50, 50)).save(path)
    assert _pipeline("top").analyze(path)["ok"] is True


def test_result_has_expected_keys(img):
    result = _pipeline("dress").analyze(img)
    assert set(result) == {
        "ok", "category", "sub_category", "color", "formality", "clothing_probability",
    }