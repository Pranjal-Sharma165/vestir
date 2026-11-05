"""Classify clothing photos with the ML pipeline and add them to the Vestir database.

Put photos in vestir-all\\wardrobe, then run from the ai-models folder:
    python import_wardrobe.py
    python import_wardrobe.py path\\to\\other\\photos --db C:\\path\\to\\vestir.db

Photos stay where they are; the database stores their full path. By default it
writes to the database the desktop app reads (%APPDATA%\\Vestir\\Vestir\\vestir.db).
Run the app once first so that database exists.
"""
import argparse
import os
import sqlite3
from pathlib import Path

from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline
from ai.classifier.fashionclip_color_detector import FashionCLIPColorDetector
from ai.classifier.formality_detector import FormalityDetector

IMAGE_EXTENSIONS = {".jpg", ".jpeg", ".png", ".heic", ".heif"}
CATEGORY_TO_DB = {"top": "Top", "bottom": "Bottom", "dress": "Dress", "outerwear": "Outerwear"}

# Must match the CHECK constraint on clothing_items.sub_category.
ALLOWED_SUB_CATEGORIES = {
    "t-shirt", "blouse", "sweater", "hoodie", "shirt",
    "jeans", "trousers", "shorts", "skirt", "leggings",
    "jacket", "coat", "blazer", "parka", "cardigan", "half-sleeve jacket",
    "dress", "jumpsuit", "romper",
}


def default_db_path() -> Path:
    return Path(os.environ["APPDATA"]) / "Vestir" / "Vestir" / "vestir.db"


DEFAULT_PHOTOS = Path(__file__).resolve().parent.parent / "wardrobe"


def convert_heic(photo: Path) -> Path:
    """Convert an iPhone HEIC photo to JPG (the app and ML need JPG/PNG)."""
    from pillow_heif import register_heif_opener
    from PIL import Image, ImageOps

    register_heif_opener()
    target = photo.with_suffix(".jpg")
    if not target.exists():
        with Image.open(photo) as img:
            ImageOps.exif_transpose(img).convert("RGB").save(target, "JPEG", quality=92)
    photo.rename(photo.with_suffix(photo.suffix + ".converted"))
    return target


def best_allowed_sub_category(sub: dict):
    """The classifier may still output labels the database does not allow."""
    for entry in sub["ranked"]:
        if entry["label"] in ALLOWED_SUB_CATEGORIES:
            return entry["label"], entry["score"]
    return None, None


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter)
    parser.add_argument("photos", type=Path, nargs="?", default=DEFAULT_PHOTOS,
                        help="Folder containing clothing photos (default: vestir-all\\wardrobe)")
    parser.add_argument("--db", type=Path, default=default_db_path(), help="Database file to write to")
    args = parser.parse_args()

    if not args.photos.is_dir():
        raise SystemExit(f"Photo folder not found: {args.photos}")
    if not args.db.is_file():
        raise SystemExit(f"Database not found: {args.db}\nRun the app once so it creates it, or pass --db.")

    images = sorted(p for p in args.photos.iterdir() if p.suffix.lower() in IMAGE_EXTENSIONS)
    if not images:
        raise SystemExit("No .jpg/.jpeg/.png files found.")

    print("Loading FashionCLIP (first run downloads the model)...")
    model = FashionCLIPModel()
    pipeline = ClothingPipeline(model)
    color_detector = FashionCLIPColorDetector(model)
    formality_detector = FormalityDetector(model)

    conn = sqlite3.connect(args.db)
    conn.execute("PRAGMA foreign_keys = ON")

    for photo in images:
        if photo.suffix.lower() in {".heic", ".heif"}:
            photo = convert_heic(photo)
        already = conn.execute(
            "SELECT 1 FROM clothing_items WHERE image_path LIKE ?", (f"%{photo.name}",)
        ).fetchone()
        if already:
            print(f"skip   {photo.name} (already imported)")
            continue

        result = pipeline.predict(photo)
        category = result["category"]
        sub_label, sub_score = best_allowed_sub_category(result["sub_category"])
        color = color_detector.predict(photo)
        formality = formality_detector.predict(photo)

        stored = photo.resolve()
        try:  # store relative to the project root so it works on any machine
            stored = stored.relative_to(DEFAULT_PHOTOS.parent).as_posix()
        except ValueError:
            pass

        conn.execute(
            """INSERT INTO clothing_items
               (category_id, item_name, image_path, sub_category, primary_color, formality,
                category_confidence, sub_category_confidence, color_confidence, formality_confidence)
               VALUES ((SELECT category_id FROM clothing_categories WHERE category_name = ?),
                       NULL, ?, ?, ?, ?, ?, ?, ?, ?)""",
            (
                CATEGORY_TO_DB[category["label"]],
                str(stored),
                sub_label,
                color["color"],
                formality["formality"],
                category["confidence"],
                sub_score,
                color["confidence"],
                formality["confidence"],
            ),
        )
        conn.commit()
        print(f"added  {photo.name}: {category['label']} / {sub_label} / {color['color']} / {formality['formality']}")

    conn.close()
    print("Done. Reopen the app to see the new items.")


if __name__ == "__main__":
    main()
