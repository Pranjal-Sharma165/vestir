import random

from .compatibility import is_valid_outfit
from .outfit_generator import generate_outfits
from .ranker import get_outfit_key, get_recommendations, rank_outfits
from .scoring_v2 import calculate_outfit_score_v2, score_breakdown

COLORS = [
    "black", "white", "gray", "charcoal", "brown", "beige", "khaki",
    "cream", "navy blue", "blue", "light blue", "green", "olive green",
    "red", "pink", "purple", "yellow", "orange", "burgundy",
    "turquoise",  # not in scoring.py -> exercises the unknown-colour branch
]
FORMALITIES = ["casual", "smart-casual", "formal", "sporty", "weird"]

SUBCATEGORIES = {
    "top": ["t-shirt", "blouse", "sweater", "hoodie", "shirt", "tank top"],
    "bottom": ["jeans", "trousers", "shorts", "skirt", "leggings"],
    "outerwear": ["jacket", "coat", "blazer", "parka", "cardigan", "half-sleeve jacket", "vest"],
    "dress": ["dress", "jumpsuit", "romper"],
}


def make_wardrobe(n_top, n_bottom, n_dress, n_outer, seed=0):
    rng = random.Random(seed)
    items, next_id = [], 1
    for cat, n in (
        ("top", n_top), ("bottom", n_bottom),
        ("dress", n_dress), ("outerwear", n_outer),
    ):
        for _ in range(n):
            # messy casing/whitespace on purpose: the code must normalise it
            name = rng.choice([cat, cat.title(), f" {cat.upper()} "])
            item = {
                "clothing_item_id": next_id,
                "category_name": name,
                "item_name": f"{cat}-{next_id}",
                "primary_color": rng.choice(COLORS),
                "formality": rng.choice(FORMALITIES),
            }
            sub = rng.choice(SUBCATEGORIES[cat] + [None, "", "mystery garment", " T_Shirt "])
            if sub is not None:
                item["sub_category"] = sub
            items.append(item)
            next_id += 1
    rng.shuffle(items)
    return items


def reference_rank(items, temperature, rain, event, excluded=None):
    """Plain brute force using the readable scoring."""
    outfits = generate_outfits(items)
    excluded = excluded or set()
    outfits = [o for o in outfits if get_outfit_key(o) not in excluded]
    for o in outfits:
        o["score"] = calculate_outfit_score_v2(o, temperature, rain, event)
    outfits.sort(key=lambda o: o["score"], reverse=True)
    return outfits


def signature(outfits):
    return [
        ([i["clothing_item_id"] for i in o["items"]], o["score"])
        for o in outfits
    ]


WARDROBES = [
    make_wardrobe(6, 5, 3, 3, seed=1),
    make_wardrobe(12, 8, 0, 4, seed=2),   # no dresses
    make_wardrobe(10, 0, 5, 2, seed=3),   # no bottoms
    make_wardrobe(8, 6, 2, 0, seed=4),    # no outerwear
    make_wardrobe(0, 0, 4, 2, seed=5),    # dresses only
    make_wardrobe(25, 15, 8, 6, seed=6),
    [],
]
CONTEXTS = [
    (t, r, e)
    for t in (-12, -5, 5, 10, 17.5, 20, 30, 38)
    for r in (0, 20, 40, 70, 95)
    for e in ("school", "work", "party", "hangout", "gym", " Work ", "unknown")
]


# ---------------------------------------------------------------- engine == reference
def test_full_ranking_matches_reference():
    for wardrobe in WARDROBES:
        for t, r, e in CONTEXTS[::3]:
            assert signature(rank_outfits(wardrobe, t, r, e)) == signature(
                reference_rank(wardrobe, t, r, e)
            ), (len(wardrobe), t, r, e)


def test_top_k_matches_reference_including_ties():
    for wardrobe in WARDROBES:
        for t, r, e in CONTEXTS[::11]:
            expected = signature(reference_rank(wardrobe, t, r, e))
            for limit in (1, 5, 20):
                got = signature(get_recommendations(wardrobe, t, r, e, limit=limit))
                assert got == expected[:limit], (len(wardrobe), t, r, e, limit)


def test_exclusions_match_reference():
    wardrobe = WARDROBES[0]
    t, r, e = 12, 30, "work"
    excluded = set()
    for _ in range(6):
        got = get_recommendations(wardrobe, t, r, e, excluded_outfits=excluded, limit=3)
        expected = reference_rank(wardrobe, t, r, e, excluded)[:3]
        assert signature(got) == signature(expected)
        excluded.update(get_outfit_key(o) for o in got)


def test_all_outputs_are_structurally_valid():
    for wardrobe in WARDROBES:
        for outfit in rank_outfits(wardrobe, 15, 50, "hangout"):
            assert is_valid_outfit(outfit)


def test_breakdown_matches_engine_score():
    wardrobe = WARDROBES[5]
    recs = get_recommendations(wardrobe, 8, 60, "school", limit=5, include_breakdown=True)
    for outfit in recs:
        assert outfit["breakdown"]["final"] == outfit["score"]


# ---------------------------------------------------------------- behaviour
def piece(item_id, category, sub, color="black", formality="casual"):
    item = {
        "clothing_item_id": item_id,
        "category_name": category,
        "item_name": f"{sub}-{item_id}",
        "primary_color": color,
        "formality": formality,
    }
    if sub:
        item["sub_category"] = sub
    return item


SMALL = [
    piece(1, "top", "t-shirt"),
    piece(2, "top", "sweater"),
    piece(3, "bottom", "shorts"),
    piece(4, "bottom", "jeans"),
    piece(5, "outerwear", "parka"),
    piece(6, "outerwear", "cardigan"),
]


def best_ids(wardrobe, t, r, e):
    return {i["clothing_item_id"] for i in get_recommendations(wardrobe, t, r, e, limit=1)[0]["items"]}


def test_freezing_day_prefers_warm_layers():
    assert best_ids(SMALL, 0, 0, "hangout") == {2, 4, 5}  # sweater + jeans + parka


def test_hot_day_prefers_light_outfit_without_outerwear():
    assert best_ids(SMALL, 32, 0, "hangout") == {1, 3}  # t-shirt + shorts


def test_heavy_rain_prefers_waterproof_outerwear():
    assert 5 in best_ids(SMALL, 15, 90, "hangout")  # parka, not the cardigan


def test_shorts_in_the_cold_rank_below_proper_layers():
    cold_wrong = {"items": [SMALL[0], SMALL[2], SMALL[5]]}  # t-shirt, shorts, cardigan
    cold_right = {"items": [SMALL[1], SMALL[3], SMALL[4]]}  # sweater, jeans, parka
    assert (
        calculate_outfit_score_v2(cold_right, 0, 0, "hangout")
        > calculate_outfit_score_v2(cold_wrong, 0, 0, "hangout")
    )


def test_gym_prefers_leggings_over_jeans():
    tee = piece(1, "top", "t-shirt")
    leggings = piece(2, "bottom", "leggings")
    jeans = piece(3, "bottom", "jeans")
    assert (
        calculate_outfit_score_v2({"items": [tee, leggings]}, 20, 0, "gym")
        > calculate_outfit_score_v2({"items": [tee, jeans]}, 20, 0, "gym")
    )


def test_hoodie_under_blazer_is_penalised():
    hoodie = piece(1, "top", "hoodie")
    shirt = piece(2, "top", "shirt")
    jeans = piece(3, "bottom", "jeans")
    blazer = piece(4, "outerwear", "blazer")
    assert score_breakdown({"items": [hoodie, jeans, blazer]}, 15, 0, "work")["style_penalty"] == 15
    assert score_breakdown({"items": [shirt, jeans, blazer]}, 15, 0, "work")["style_penalty"] == 0


def test_bare_arms_are_penalised_when_cold_only():
    tank = piece(1, "top", "tank top")
    tee = piece(2, "top", "t-shirt")
    sweater = piece(3, "top", "sweater")
    jeans = piece(4, "bottom", "jeans")
    vest = piece(5, "outerwear", "vest")

    def arm(items, temp):
        return score_breakdown({"items": items}, temp, 0, "hangout")["arm_penalty"]

    assert arm([tank, jeans, vest], 10) == 16    # bare arms, needs long sleeves
    assert arm([tee, jeans], 10) == 8            # short sleeves, needs long
    assert arm([sweater, jeans, vest], 10) == 0  # sweater covers the arms
    assert arm([tank, jeans, vest], 25) == 0     # warm: bare arms are fine
    assert arm([tee, jeans], 15) == 0            # 15 C only needs short sleeves


def test_cold_day_prefers_sleeves_over_tank_and_vest():
    wardrobe = [
        piece(1, "top", "tank top"),
        piece(2, "top", "sweater"),
        piece(3, "bottom", "jeans"),
        piece(4, "outerwear", "vest"),
    ]
    best = best_ids(wardrobe, 10, 0, "hangout")
    assert 2 in best and 1 not in best


def test_items_without_subcategory_still_work():
    bare = [piece(i, cat, None) for i, cat in enumerate(
        ["top", "top", "bottom", "bottom", "dress", "outerwear"], start=1)]
    recs = get_recommendations(bare, 12, 40, "work", limit=3)
    assert len(recs) == 3
    assert all(is_valid_outfit(o) for o in recs)


def test_diversity_reduces_overlap():
    wardrobe = WARDROBES[5]
    plain = get_recommendations(wardrobe, 12, 30, "work", limit=5)
    varied = get_recommendations(wardrobe, 12, 30, "work", limit=5, diversity=5.0)
    assert len(varied) == 5

    def overlap(outfits):
        keys = [get_outfit_key(o) for o in outfits]
        return sum(len(a & b) for i, a in enumerate(keys) for b in keys[i + 1:])

    assert overlap(varied) <= overlap(plain)


def test_pruning_returns_valid_outfits():
    wardrobe = WARDROBES[5]
    recs = get_recommendations(wardrobe, 12, 30, "work", limit=5, max_per_category=5)
    assert len(recs) == 5
    assert all(is_valid_outfit(o) for o in recs)


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_") and callable(fn):
            fn()
            print("ok", name)
            