"""
uvicorn app:app --host 127.0.0.1 --port 8001
"""
import tempfile
import threading
from contextlib import asynccontextmanager
from io import BytesIO
from pathlib import Path
from typing import Optional

from fastapi import FastAPI, File, HTTPException, UploadFile
from PIL import Image
from pydantic import BaseModel, ConfigDict, Field

from ai.recommendation2.ranker import get_recommendations
from ai.classifier.fashionclip_model import FashionCLIPModel
from ai.classifier.pipeline import ClothingPipeline

models = {}
model_lock = threading.Lock() 


@asynccontextmanager
async def lifespan(app):
    shared = FashionCLIPModel()
    models["pipeline"] = ClothingPipeline(shared)
    yield
    models.clear()


app = FastAPI(lifespan=lifespan)

# curl -X POST http://127.0.0.1:8001/analyze -F "image=@uploads/image.jpg"
@app.post("/analyze")
def analyze(image: UploadFile = File(...)):
    try:
        img = Image.open(BytesIO(image.file.read())).convert("RGB")
    except Exception:
        raise HTTPException(422, "Invalid image")

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "item.jpg"
        img.save(path, "JPEG", quality=90)
        with model_lock:
            result = models["pipeline"].analyze(path)

    if not result["ok"]:
        raise HTTPException(422, result["error"])

    return {
        "category_name": result["category"]["label"],
        "sub_category": result["sub_category"]["label"],
        "primary_color": result["color"]["color"],
        "formality": result["formality"]["formality"],
        "confidence": {
            "category": result["category"]["confidence"],
            "sub_category": result["sub_category"]["confidence"],
            "color": result["color"]["confidence"],
            "formality": result["formality"]["confidence"],
        },
    }


class Item(BaseModel):
    model_config = ConfigDict(extra="allow")

    clothing_item_id: int
    category_name: str
    sub_category: Optional[str] = None
    primary_color: str
    formality: str


class RecommendationRequest(BaseModel):
    temperature: float
    rain_probability: float = Field(ge=0, le=100)
    event: str
    items: list[Item]
    excluded_outfits: list[list[int]] = []
    limit: int = Field(5, ge=1, le=50)
    diversity: float = Field(0.0, ge=0)
    include_breakdown: bool = False


# curl -X POST http://127.0.0.1:8001/recommendations -H "Content-Type: application/json" -d @request.json
@app.post("/recommendations")
def recommend(req: RecommendationRequest):
    items = [item.model_dump() for item in req.items]

    excluded = {frozenset(ids) for ids in req.excluded_outfits}

    outfits = get_recommendations(
        items,
        req.temperature,
        req.rain_probability,
        req.event,
        excluded_outfits=excluded,
        limit=req.limit,
        diversity=req.diversity,
        include_breakdown=req.include_breakdown,
    )

    return {
        "outfits": [
            {
                "item_ids": sorted(i["clothing_item_id"] for i in o["items"]),
                "score": o["score"],
                "items": o["items"],
                **({"breakdown": o["breakdown"]} if req.include_breakdown else {}),
            }
            for o in outfits
        ]
    }


@app.get("/health")
def health():
    return {"ok": True, "models_loaded": bool(models)}