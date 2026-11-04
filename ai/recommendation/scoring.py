from itertools import combinations


# Neutral colours
NEUTRALS = {
    "black", "white", "gray", "charcoal",
    "beige", "khaki", "cream", "navy blue"
}

# Warm colours
WARM_COLORS = {
    "brown", "red", "pink", "yellow",
    "orange", "burgundy"
}

# Cool colours
COOL_COLORS = {
    "blue", "light blue", "green",
    "olive green", "purple"
}


def normalize_color(color: str) -> str:
    return color.strip().lower()


def get_color_pair_score(color1: str, color2: str) -> int:
    c1 = normalize_color(color1)
    c2 = normalize_color(color2)

    # Same colour
    if c1 == c2:
        return 95

    # Both neutral
    if c1 in NEUTRALS and c2 in NEUTRALS:
        return 98

    # One neutral
    if c1 in NEUTRALS or c2 in NEUTRALS:
        return 92

    # Both warm or both cool
    if (
        (c1 in WARM_COLORS and c2 in WARM_COLORS)
        or
        (c1 in COOL_COLORS and c2 in COOL_COLORS)
    ):
        return 88

    # Warm and cool combination
    if (
        (c1 in WARM_COLORS and c2 in COOL_COLORS)
        or
        (c1 in COOL_COLORS and c2 in WARM_COLORS)
    ):
        return 72

    # Unknown colour
    return 75


def calculate_color_score(outfit: dict) -> float:
    items = outfit["items"]

    if len(items) < 2:
        return 100.0

    scores = []

    for item1, item2 in combinations(items, 2):
        score = get_color_pair_score(
            item1["primary_color"],
            item2["primary_color"]
        )
        scores.append(score)

    return round(sum(scores) / len(scores), 2)

# formality score
# Formality compatibility scores
FORMALITY_COMPATIBILITY = {
    ("casual", "casual"): 95,
    ("smart-casual", "smart-casual"): 95,
    ("formal", "formal"): 95,
    ("sporty", "sporty"): 95,

    ("casual", "smart-casual"): 85,
    ("formal", "smart-casual"): 85,

    ("casual", "formal"): 60,
    ("formal", "sporty"): 40,
    ("casual", "sporty"): 75,
    ("smart-casual", "sporty"): 60,
}


def get_formality_pair_score(formality1: str, formality2: str) -> int:
    f1 = formality1.strip().lower()
    f2 = formality2.strip().lower()

    pair = tuple(sorted([f1, f2]))

    return FORMALITY_COMPATIBILITY.get(pair, 70)


def calculate_formality_score(outfit: dict) -> float:
    items = outfit["items"]

    if len(items) < 2:
        return 100.0

    scores = []

    for item1, item2 in combinations(items, 2):
        score = get_formality_pair_score(
            item1["formality"],
            item2["formality"]
        )
        scores.append(score)

    return round(sum(scores) / len(scores), 2)

# temperature score
def calculate_temperature_score(outfit: dict, temperature: float) -> float:
    items = outfit["items"]

    has_outerwear = any(
        item["category_name"].strip().lower() == "outerwear"
        for item in items
    )

    if temperature <= 5:
        return 90 if has_outerwear else 45

    elif temperature <= 15:
        return 95 if has_outerwear else 75

    elif temperature <= 25:
        return 95

    else:
        return 75 if has_outerwear else 95

def calculate_rain_score(outfit: dict, rain_probability: float) -> float:
    items = outfit["items"]

    has_outerwear = any(
        item["category_name"].strip().lower() == "outerwear"
        for item in items
    )

    if rain_probability <= 20:
        return 100

    elif rain_probability <= 50:
        return 90

    elif rain_probability <= 80:
        return 80 if has_outerwear else 70

    else:
        return 75 if has_outerwear else 65

def calculate_weather_score(
    outfit: dict,
    temperature: float,
    rain_probability: float
) -> float:
    temperature_score = calculate_temperature_score(
        outfit, temperature
    )

    rain_score = calculate_rain_score(
        outfit, rain_probability
    )

    return round((temperature_score + rain_score) / 2, 2)

# event score
# Event compatibility scores
EVENT_COMPATIBILITY = {
    "school": {
        "casual": 95,
        "smart-casual": 85,
        "formal": 50,
        "sporty": 75,
    },
    "work": {
        "casual": 65,
        "smart-casual": 95,
        "formal": 95,
        "sporty": 30,
    },
    "party": {
        "casual": 75,
        "smart-casual": 85,
        "formal": 90,
        "sporty": 65,
    },
    "hangout": {
        "casual": 95,
        "smart-casual": 85,
        "formal": 60,
        "sporty": 85,
    },
    "gym": {
        "casual": 60,
        "smart-casual": 30,
        "formal": 10,
        "sporty": 100,
    },
}


def calculate_event_score(outfit: dict, event: str) -> float:
    event = event.strip().lower()

    if event not in EVENT_COMPATIBILITY:
        return 75.0

    items = outfit["items"]

    if not items:
        return 0.0

    scores = []

    for item in items:
        formality = item["formality"].strip().lower()

        score = EVENT_COMPATIBILITY[event].get(formality, 75)
        scores.append(score)

    return round(sum(scores) / len(scores), 2)

# overall score calculation
def calculate_outfit_score(
    outfit: dict,
    temperature: float,
    rain_probability: float,
    event: str
) -> float:

    color_score = calculate_color_score(outfit)
    formality_score = calculate_formality_score(outfit)

    weather_score = calculate_weather_score(
        outfit, temperature, rain_probability
    )

    event_score = calculate_event_score(outfit, event)

    final_score = (
        color_score      * 0.25 +
        formality_score  * 0.10 +   # low influence
        weather_score    * 0.30 +
        event_score      * 0.35
    )

    return round(final_score, 2)