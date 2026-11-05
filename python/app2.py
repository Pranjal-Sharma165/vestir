"""
uvicorn app2:app --host 127.0.0.1 --port 8001
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
from ai.classifier2.fashionclip_model import FashionCLIPModel
from ai.classifier2.pipeline import ClothingPipeline

models = {}
model_lock = threading.Lock() 


@asynccontextmanager
async def lifespan(app):
    shared = FashionCLIPModel()
    models["pipeline"] = ClothingPipeline(shared)  # use_detector=True for outfit photos
    yield
    models.clear()


app = FastAPI(lifespan=lifespan)


# curl -X POST http://127.0.0.1:8001/analyze -F "image=@ai/images/image.jpg"
@app.post("/analyze")
def analyze(image: UploadFile = File(...)):
    try:
        img = Image.open(BytesIO(image.file.read())).convert("RGB")
    except Exception:
        raise HTTPException(400, "File is not a valid image")

    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / "item.jpg"
        img.save(path, "JPEG", quality=90)
        with model_lock:
            return models["pipeline"].predict_one(path)


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