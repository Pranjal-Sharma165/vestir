"""
Garment detector using a clothing-specialized YOLOv8 model.
Falls back to a general YOLOv8 if the specialized weights are unavailable.
"""

from __future__ import annotations

from typing import List, Tuple, Optional, Dict
from PIL import Image
import torch
import numpy as np


class GarmentDetector:
    """
    Detects and crops clothing items from a full photo.
    Categories: Clothing, Shoes, Bags, Accessories (from the fine-tuned model).
    """

    def __init__(
        self,
        model_name: str = "kesimeg/yolov8n-clothing-detection",
        conf_threshold: float = 0.35,
        device: Optional[str] = None,
    ):
        self.conf_threshold = conf_threshold
        self.device = device or ("cuda" if torch.cuda.is_available() else "cpu")

        try:
            from ultralytics import YOLO
            print(f"[GarmentDetector] Loading {model_name} ...")
            self.model = YOLO(model_name)
            self.model.to(self.device)
            print("[GarmentDetector] Ready.")
        except Exception as e:
            print(f"[GarmentDetector] Could not load specialized model ({e}). "
                  "Falling back to yolov8n.pt (general).")
            from ultralytics import YOLO
            self.model = YOLO("yolov8n.pt")
            self.model.to(self.device)

    def detect_and_crop(
        self,
        image: Image.Image,
        max_items: int = 8,
    ) -> List[Dict]:
        """
        Returns a list of dicts:
        {
            "crop": PIL.Image,
            "bbox": (x1, y1, x2, y2),
            "label": str,
            "confidence": float
        }
        """
        # ultralytics accepts PIL directly
        results = self.model.predict(
            source=image,
            conf=self.conf_threshold,
            verbose=False,
        )

        items = []
        if not results or len(results) == 0:
            # fallback: return the whole image as one item
            return [{
                "crop": image,
                "bbox": (0, 0, image.width, image.height),
                "label": "clothing",
                "confidence": 1.0,
            }]

        result = results[0]
        boxes = result.boxes

        if boxes is None or len(boxes) == 0:
            return [{
                "crop": image,
                "bbox": (0, 0, image.width, image.height),
                "label": "clothing",
                "confidence": 1.0,
            }]

        # Sort by confidence descending
        confs = boxes.conf.cpu().numpy()
        order = np.argsort(-confs)

        for idx in order[:max_items]:
            box = boxes.xyxy[idx].cpu().numpy().astype(int)
            x1, y1, x2, y2 = box
            # clamp
            x1, y1 = max(0, x1), max(0, y1)
            x2, y2 = min(image.width, x2), min(image.height, y2)

            if x2 <= x1 or y2 <= y1:
                continue

            crop = image.crop((x1, y1, x2, y2))
            label = result.names[int(boxes.cls[idx])] if hasattr(result, "names") else "clothing"
            conf = float(confs[idx])

            items.append({
                "crop": crop,
                "bbox": (x1, y1, x2, y2),
                "label": label,
                "confidence": conf,
            })

        return items