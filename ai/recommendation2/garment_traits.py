# Change this one line if your database/API uses a different field name.
SUBCATEGORY_FIELD = "sub_category"


def canon(name):
    """'T-Shirt ', 't_shirt' and 'half-sleeve jacket' all compare reliably."""
    return " ".join(
        str(name).strip().lower().replace("-", " ").replace("_", " ").split()
    )


def _canon_keys(table):
    return {canon(k): v for k, v in table.items()}


_ALIASES = _canon_keys({
    "tshirt": "t-shirt",
    "t shirt": "t-shirt",
    "tee": "t-shirt",
    "tee shirt": "t-shirt",
    "pants": "trousers",
    "jumper": "sweater",
})
_ALIASES = {canon(k): canon(v) for k, v in _ALIASES.items()}

# ---------------------------------------------------------------------------
# Warmth: rough insulation points. An outfit's warmth is the SUM of its items,
# and is compared with the warmth the weather calls for (see scoring_v2.py).
# ---------------------------------------------------------------------------
WARMTH = _canon_keys({
    # tops
    "t-shirt": 1.0, "blouse": 1.0, "shirt": 1.5, "sweater": 3.0, "hoodie": 3.0,
    # bottoms
    "shorts": 0.5, "skirt": 1.0, "leggings": 2.0, "jeans": 2.5, "trousers": 2.5,
    # one-pieces (counted as top + bottom together)
    "romper": 1.5, "dress": 3.0, "jumpsuit": 3.5,
    # outerwear
    "half-sleeve jacket": 1.0, "cardigan": 1.5, "blazer": 1.5,
    "jacket": 2.5, "coat": 4.0, "parka": 5.0,
})

# Used when an item has no (or an unknown) sub-category.
CATEGORY_DEFAULT_WARMTH = {"top": 1.5, "bottom": 2.0, "dress": 3.0, "outerwear": 2.5}

# ---------------------------------------------------------------------------
# Rain protection of outerwear: 0 = none, 1 = fully waterproof.
# ---------------------------------------------------------------------------
RAIN_PROTECTION = _canon_keys({
    "parka": 1.0, "coat": 0.8, "jacket": 0.7,
    "half-sleeve jacket": 0.3, "blazer": 0.25, "cardigan": 0.1,
})
DEFAULT_OUTERWEAR_RAIN_PROTECTION = 0.5

# ---------------------------------------------------------------------------
# Event adjustments (points added to the item's event score, result is kept
# within 0-100). They refine the formality-based event score using what the
# garment actually is.
# ---------------------------------------------------------------------------
EVENT_ADJUSTMENTS = {
    "work": _canon_keys({
        "shorts": -25, "hoodie": -25, "leggings": -25, "romper": -20,
        "t-shirt": -10, "half-sleeve jacket": -10, "jeans": -5,
        "blazer": 5, "shirt": 5, "blouse": 5, "trousers": 5,
    }),
    "school": _canon_keys({
        "blazer": -5, "jumpsuit": -5, "hoodie": 5, "jeans": 5, "t-shirt": 5,
    }),
    "party": _canon_keys({
        "hoodie": -15, "leggings": -10, "shorts": -10, "t-shirt": -10,
        "parka": -10, "dress": 5, "jumpsuit": 5, "blouse": 5, "blazer": 5,
    }),
    "hangout": _canon_keys({
        "blazer": -5, "jumpsuit": -5, "hoodie": 5, "jeans": 5,
    }),
    "gym": _canon_keys({
        "leggings": 15, "shorts": 10, "t-shirt": 10, "hoodie": 5,
        "jeans": -20, "trousers": -20, "shirt": -20, "skirt": -25,
        "cardigan": -15, "sweater": -10, "coat": -25, "blouse": -30,
        "romper": -30, "blazer": -40, "dress": -40, "jumpsuit": -40,
    }),
}

# ---------------------------------------------------------------------------
# Layering clashes: (top, outerwear) -> points taken off the final score.
# ---------------------------------------------------------------------------
TOP_OUTERWEAR_PENALTY = {
    (canon(top), canon(outer)): points
    for (top, outer), points in {
        ("hoodie", "blazer"): 15,
        ("hoodie", "cardigan"): 6,
        ("sweater", "cardigan"): 6,
        ("t-shirt", "half-sleeve jacket"): 10,
    }.items()
}


# ---------------------------------------------------------------------------
# Lookups
# ---------------------------------------------------------------------------
def get_category(item):
    return item["category_name"].strip().lower()


def get_subcategory(item):
    raw = item.get(SUBCATEGORY_FIELD)

    if not raw:
        return None

    if isinstance(raw, dict):
        raw = raw.get("label")

    if not raw:
        return None

    name = canon(raw)
    return _ALIASES.get(name, name)


def get_warmth(item):
    sub = get_subcategory(item)
    if sub in WARMTH:
        return WARMTH[sub]
    return CATEGORY_DEFAULT_WARMTH.get(get_category(item), 2.0)


def get_rain_protection(item):
    """Rain protection an outerwear item gives (0 for anything else)."""
    if get_category(item) != "outerwear":
        return 0.0
    return RAIN_PROTECTION.get(
        get_subcategory(item), DEFAULT_OUTERWEAR_RAIN_PROTECTION
    )


def get_event_adjustment(item, event):
    return EVENT_ADJUSTMENTS.get(canon(event), {}).get(get_subcategory(item), 0)


def get_top_outerwear_penalty(top_item, outerwear_item):
    return TOP_OUTERWEAR_PENALTY.get(
        (get_subcategory(top_item), get_subcategory(outerwear_item)), 0
    )
