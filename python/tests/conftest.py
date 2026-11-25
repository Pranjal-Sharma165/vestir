import os

import pytest


def pytest_configure(config):
    config.addinivalue_line(
        "markers",
        "model: loads the real FashionCLIP weights (slow, needs a download); "
        "skipped unless RUN_MODEL_TESTS=1",
    )


def pytest_collection_modifyitems(config, items):
    if os.environ.get("RUN_MODEL_TESTS") == "1":
        return
    skip = pytest.mark.skip(reason="set RUN_MODEL_TESTS=1 to run real-model tests")
    for item in items:
        if "model" in item.keywords:
            item.add_marker(skip)
            