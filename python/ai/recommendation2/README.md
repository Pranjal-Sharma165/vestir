# Vestir Outfit Recommender (`recommendation2`)

Ranks every valid outfit a user can build from their wardrobe for a given **temperature**, **chance of rain** and **event**, and returns the best few with an optional per-component score breakdown. It also maps free-text calendar titles ("Midterm exam", "Coffee with Sam") onto the five events the recommender understands.

It is rule-based (no training). Every number lives in a small table, so the scoring is explainable and easy to tune.

## Quick start

```python
from ai.recommendation2.ranker import get_recommendations
from ai.recommendation2.event_mapper import map_event

event = map_event("Team stand-up").event          # "work"

recs = get_recommendations(
    clothing_items,            # list of item dicts, see "Input format"
    temperature=10,            # °C
    rain_probability=60,       # %
    event=event,
    limit=5,
    include_breakdown=True,
)

for outfit in recs:
    print(outfit["score"], [i["item_name"] for i in outfit["items"]])
    print(outfit["breakdown"])
```

### Using it with the classifier output

`ClothingPipeline.analyze()` results map directly onto the item format:

```python
r = pipeline.analyze(path)
if r["ok"]:
    clothing_items.append({
        "clothing_item_id": next_id,
        "item_name": path.stem,
        "category_name": r["category"]["label"],
        "sub_category": r["sub_category"]["label"],
        "primary_color": r["color"]["color"],
        "formality": r["formality"]["formality"],
    })
```

## Input format

Each wardrobe item is a dict:

| Field | Required | Notes |
|---|---|---|
| `clothing_item_id` | yes | Unique per item. Used for outfit keys and exclusions. |
| `category_name` | yes | `top`, `bottom`, `dress` or `outerwear`. Case and surrounding whitespace are ignored. Items with any other category are skipped. |
| `primary_color` | yes | One of the 19 classifier colours. Unknown colours get a neutral-ish score instead of failing. |
| `formality` | yes | `casual`, `smart-casual`, `formal` or `sporty`. Unknown values get a default score. |
| `sub_category` | no | e.g. `jeans`, `parka`. Can be a string or a dict with a `"label"` key. Without it the item falls back to category defaults. |
| `item_name` | no | Only used for display. |

The sub-category field name is configurable with `SUBCATEGORY_FIELD` in `garment_traits.py`. Names are normalised (`" T_Shirt "`, `tshirt`, `tee` → `t-shirt`; `pants` → `trousers`; `jumper` → `sweater`).

## What counts as an outfit

Outfits are built so they are valid by construction:

- **Top + Bottom**, optionally **+ one Outerwear**
- **Dress**, optionally **+ one Outerwear**

For `t` tops, `b` bottoms, `d` dresses and `o` outerwear items, that is `t·b·(o+1) + d·(o+1)` outfits.

## How an outfit is scored

```
final = 0.25 · colour
      + 0.10 · formality
      + 0.30 · weather
      + 0.35 · event
      − style_penalty − arm_penalty
```

Weights are `W_COLOR`, `W_FORMALITY`, `W_WEATHER`, `W_EVENT` in `scoring_v2.py`.

### Colour (`scoring.py`)

Every pair of items gets a score, and the outfit takes the average (a lone dress scores 100).

| Pair | Score |
|---|---|
| Both neutral | 98 |
| Same colour | 95 |
| One neutral | 92 |
| Both warm, or both cool | 88 |
| One warm, one cool | 82 |
| Unknown colour | 75 |

Neutrals: black, white, gray, charcoal, beige, khaki, cream, navy blue, blue, light blue. Warm: brown, red, pink, yellow, orange, burgundy. Cool: green, olive green, purple.

### Formality match (`scoring.py`)

Pairwise again, averaged. Same formality scores 95, casual/smart-casual and formal/smart-casual 85, casual/sporty 75, casual/formal 60, smart-casual/sporty 60, formal/sporty 40, anything else 70.

### Event (`scoring_v2.py`, `garment_traits.py`)

Each garment gets a base score from its formality for the event (`EVENT_COMPATIBILITY`, e.g. `sporty` is 100 for gym but 30 for work), plus an adjustment for what the garment actually is (`EVENT_ADJUSTMENTS`: leggings +15 at the gym, shorts −25 at work, blazer −40 at the gym). The result is clamped to 0–100 and the outfit takes the mean over its items. Unknown events score 75.

Supported events: `school`, `work`, `party`, `hangout`, `gym`.

### Weather

`weather = temperature_score × rain_score / 100`

**Temperature.** Each garment has a warmth value (t-shirt 1.0, sweater 3.0, jeans 2.5, parka 5.0, ...) and the outfit's warmth is the **sum**. The weather calls for a target warmth, linearly interpolated between these points:

| °C | −10 | 0 | 5 | 10 | 15 | 20 | 25 | 30 | 35 |
|---|---|---|---|---|---|---|---|---|---|
| Target warmth | 13 | 10.5 | 9 | 7.5 | 6 | 4.5 | 3.5 | 2.5 | 1.5 |

Within ±0.75 of the target there is no penalty. Beyond that, each warmth point costs 12 points if underdressed or 8 if overdressed (being cold is worse), with a floor of 10.

**Rain.** Below 20% rain probability, rain is ignored. Above it, the penalty scales up to 60 points at 100%, multiplied by `(1 − protection)`. Only outerwear gives protection (parka 1.0, coat 0.8, jacket 0.7, half-sleeve jacket 0.3, vest 0.3, blazer 0.25, cardigan 0.1, unknown outerwear 0.5).

### Penalties

- **Arm coverage.** Arm coverage is 0 (bare), 1 (short sleeve) or 2 (long sleeve), taking the best of the top and the outerwear. Below 20 °C, the coverage the weather needs rises by 1 level per 5 °C, reaching long sleeves at 10 °C. Each missing level costs 8 points. (A tank top plus vest at 10 °C loses 16.)
- **Style clashes** (top + outerwear): hoodie + blazer −15, t-shirt + half-sleeve jacket −10, hoodie + cardigan −6, sweater + cardigan −6.

Both penalties apply to top + bottom outfits only. Dress outfits are not affected.

### Missing sub-category

Items without a (known) sub-category still work and get category defaults: warmth top 1.5, bottom 2.0, dress 3.0, outerwear 2.5; long-sleeve arm coverage (no penalty); outerwear rain protection 0.5; no event adjustment.

## API

### `get_recommendations(...)` in `ranker.py`

```python
get_recommendations(
    clothing_items, temperature, rain_probability, event,
    excluded_outfits=None, limit=5, diversity=0.0,
    max_per_category=None, include_breakdown=False,
)
```

Returns a list of `{"items": [...], "score": float}` sorted from best to worst, with `"breakdown"` added when requested.

| Parameter | Meaning |
|---|---|
| `excluded_outfits` | Set of `frozenset`s of item ids to skip (outfits already shown). Use `get_outfit_key(outfit)` to build them. |
| `limit` | Number of outfits to return. |
| `diversity` | `0` keeps pure score order. A value like `3` subtracts 3 points per item shared with an outfit already chosen, so the top results are not near-copies. Re-ranks a candidate pool of `max(limit·10, 50)`. |
| `max_per_category` | Keep only the best N items per category (by event score) before combining. **Approximate**: use only on huge wardrobes. |
| `include_breakdown` | Adds the score components for "why this outfit?" displays. |

Breakdown keys: `color`, `formality`, `weather`, `temperature`, `rain`, `outfit_warmth`, `target_warmth`, `event`, `arm_penalty`, `style_penalty`, `final`. `breakdown["final"]` always equals the outfit's `score`.

Other helpers: `rank_outfits(...)` (same arguments, `limit=None` returns every outfit), `diversify(...)`, `get_outfit_key(outfit)`.

### Event mapper (`event_mapper.py`)

```python
from ai.recommendation2.event_mapper import map_event, EventMapper

map_event("Birthdy dinner")
# EventMatch(event="party", confidence=..., method="fuzzy", matched=("birthday", ...))
```

`EventMatch` has `event` (always one of the five), `confidence` (0.0 to 1.0), `method` and `matched`. The mapper tries these in order and stops at the first that matches:

| Step | `method` | How |
|---|---|---|
| 1. Learned | `learned` | Exact normalised title previously taught with `mapper.learn(title, event)`. Confidence 1.0. |
| 2. Keywords | `keyword` | Multi-word phrases (weight 2) and single words (weight 1), with simple plural handling. Highest total wins; confidence is the winner's share of the total. Ties go to the earlier event in `EVENT_KEYWORDS` order. |
| 3. Fuzzy | `fuzzy` | Typo tolerance for words of 4+ characters (`difflib`, cutoff 0.88, weight 0.5). |
| 4. Semantic | `semantic` | Optional. Pass `embed_fn` (e.g. `shared_model.encode_text`) and the title is compared with prototype sentences per event. Titles closest to the `other` prototype (dentist, flights) are not forced into an event. |
| 5. Default | `default` / `empty` | `hangout` with confidence 0.0. |

A confidence of 0 means the app guessed, so consider asking the user to pick the event. To teach the mapper from user corrections, call `mapper.learn("Dentist appointment", "work")`.

```python
mapper = EventMapper(embed_fn=pipeline.model.encode_text)   # optional semantic fallback
```

## Package layout

| File | Purpose |
|---|---|
| `ranker.py` | Public entry point (`get_recommendations`), diversification, exclusions. |
| `outfit_engine.py` | Fast outfit generation and scoring: pairwise colour and formality matrices precomputed per category pair, results cached, and a bounded heap (`nlargest`) keeps memory flat when only the top results are needed. |
| `scoring_v2.py` | Weights, temperature/rain/arm scoring, `score_breakdown`. |
| `scoring.py` | Colour and formality pair tables and `EVENT_COMPATIBILITY`. Its older whole-outfit weather and total-score functions belong to v1 and are not used by the v2 engine. |
| `garment_traits.py` | Per-sub-category tables (warmth, rain protection, arm coverage, event adjustments, layering penalties) and alias handling. |
| `event_mapper.py` | Calendar title to event mapping. |
| `compatibility.py` | `is_valid_outfit` structural rules. Used by tests as a check on engine output. |
| `outfit_generator.py` | Simple brute-force outfit generation. Used as the **reference implementation** in tests and benchmarks, not by the engine. |
| `benchmark_recommender.py` | Timing comparison of reference vs engine vs engine with pruning. |
| `test_event_mapper.py`, `test_outfit_engine.py` | Tests, kept next to the code. |

## Tuning

| To change | Edit |
|---|---|
| How much colour, weather and event matter | `W_*` in `scoring_v2.py` |
| How warm it should be at each temperature | `TARGET_WARMTH_POINTS`, `WARMTH_TOLERANCE`, penalty rates in `scoring_v2.py` |
| Rain sensitivity | `RAIN_FREE_BELOW`, `MAX_RAIN_PENALTY` in `scoring_v2.py` |
| How warm / waterproof / long-sleeved a garment is | `WARMTH`, `RAIN_PROTECTION`, `ARM_COVERAGE` in `garment_traits.py` |
| What suits an event | `EVENT_COMPATIBILITY` in `scoring.py`, `EVENT_ADJUSTMENTS` in `garment_traits.py` |
| Which items look wrong together | `TOP_OUTERWEAR_PENALTY` in `garment_traits.py` |
| Calendar title keywords | `EVENT_KEYWORDS` in `event_mapper.py` |

If you add a sub-category to the classifier, add it to `WARMTH` (and, where relevant, `ARM_COVERAGE`, `RAIN_PROTECTION`, `EVENT_ADJUSTMENTS`). Otherwise it silently falls back to the category defaults.

## Testing

From `python/`:

```bash
python -m pytest ai/recommendation2 -v
```

(Or plain `python -m pytest` if `testpaths` in `pytest.ini` includes `ai`.)

`test_outfit_engine.py` does two kinds of checks:

1. **Engine equals reference.** Randomised wardrobes (including messy casing, missing sub-categories, unknown colours and formalities, and wardrobes with no dresses, bottoms, outerwear or items at all) are ranked by the fast engine and by the brute-force reference. Full rankings, top-k results (including ties), and exclusions must match exactly.
2. **Behaviour.** A freezing day prefers warm layers, a hot day drops outerwear, heavy rain prefers the parka over the cardigan, shorts in the cold rank below proper layers, the gym prefers leggings over jeans, hoodie under a blazer is penalised, bare arms are penalised only when cold, and diversity reduces overlap.

`test_event_mapper.py` covers normalisation, each event's titles, typo handling, confidence, learned mappings, and the semantic fallback (using a fake embedding function, so no model is needed).

## Benchmark

```bash
python -m ai.recommendation2.benchmark_recommender
```

Compares the brute-force reference, the engine, and the engine with `max_per_category=30` across wardrobe sizes up to 200 tops / 120 bottoms / 50 dresses / 50 outerwear, and asserts that scores match the reference wherever the reference is run.

## Limitations

- Warmth, rain protection and sleeve length come from the sub-category alone. Fabric, thickness and fit are invisible to the model.
- The score has no notion of personal taste, season or recently worn items. `excluded_outfits` and `diversity` are the only variety controls.
- Temperature is in °C and rain probability in %. Nothing converts units for you.
- Dress outfits skip the arm-coverage and style penalties.
- `max_per_category` trades accuracy for speed. The best outfit can be missed if it uses an item that ranked outside the cut.
- Event scoring depends on the classifier's formality label, which is a zero-shot guess. A wrong formality shifts the event and formality scores.