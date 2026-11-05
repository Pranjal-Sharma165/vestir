from . import garment_traits as traits
from .scoring import (
    EVENT_COMPATIBILITY,
    calculate_color_score,
    calculate_formality_score,
)

# Same weights as before.
W_COLOR = 0.25
W_FORMALITY = 0.10
W_WEATHER = 0.30
W_EVENT = 0.35

# (temperature in C, outfit warmth the weather calls for), linearly interpolated.
TARGET_WARMTH_POINTS = [
    (-10, 13.0), (0, 10.5), (5, 9.0), (10, 7.5), (15, 6.0),
    (20, 4.5), (25, 3.5), (30, 2.5), (35, 1.5),
]
WARMTH_TOLERANCE = 0.75      # warmth points of slack either way
COLD_PENALTY_PER_POINT = 12  # being underdressed is worse than overdressed
HOT_PENALTY_PER_POINT = 8
MIN_TEMPERATURE_SCORE = 10

RAIN_FREE_BELOW = 20         # % probability below which rain is ignored
MAX_RAIN_PENALTY = 60        # points lost at 100% rain with no protection

UNKNOWN_EVENT_SCORE = 75


def target_warmth(temperature):
    points = TARGET_WARMTH_POINTS
    if temperature <= points[0][0]:
        return points[0][1]
    if temperature >= points[-1][0]:
        return points[-1][1]
    for (t0, w0), (t1, w1) in zip(points, points[1:]):
        if temperature <= t1:
            return w0 + (w1 - w0) * (temperature - t0) / (t1 - t0)


def temperature_score(total_warmth, temperature):
    diff = total_warmth - target_warmth(temperature)
    if diff < 0:
        excess, rate = max(0.0, -diff - WARMTH_TOLERANCE), COLD_PENALTY_PER_POINT
    else:
        excess, rate = max(0.0, diff - WARMTH_TOLERANCE), HOT_PENALTY_PER_POINT
    return round(max(MIN_TEMPERATURE_SCORE, 100 - rate * excess), 2)


def rain_score(rain_probability, protection=0.0):
    need = (rain_probability - RAIN_FREE_BELOW) / (100 - RAIN_FREE_BELOW)
    need = min(1.0, max(0.0, need))
    return round(100 - MAX_RAIN_PENALTY * need * (1 - protection), 2)


def combine_weather(temp_score, rain):
    return round(temp_score * rain / 100, 2)


def item_event_score(item, event):
    """Event suitability of one garment: formality-based score + adjustment."""
    key = traits.canon(event)
    table = EVENT_COMPATIBILITY.get(key)
    if table is None:
        base = UNKNOWN_EVENT_SCORE
    else:
        base = table.get(item["formality"].strip().lower(), UNKNOWN_EVENT_SCORE)
    adjusted = base + traits.get_event_adjustment(item, key)
    return min(100, max(0, adjusted))


def combine_scores(color, formality, weather, event, penalty=0.0):
    return round(
        color * W_COLOR
        + formality * W_FORMALITY
        + weather * W_WEATHER
        + event * W_EVENT
        - penalty,
        2,
    )


def score_breakdown(outfit, temperature, rain_probability, event):
    """Every component of an outfit's score (handy for 'why this outfit?')."""
    items = outfit["items"]

    by_category = {}
    for item in items:
        by_category[traits.get_category(item)] = item
    outerwear = by_category.get("outerwear")
    top = by_category.get("top")

    warmth = sum(traits.get_warmth(item) for item in items)
    temp = temperature_score(warmth, temperature)
    protection = traits.get_rain_protection(outerwear) if outerwear else 0.0
    rain = rain_score(rain_probability, protection)

    event_scores = [item_event_score(item, event) for item in items]
    event_score = round(sum(event_scores) / len(event_scores), 2)

    penalty = (
        traits.get_top_outerwear_penalty(top, outerwear)
        if top and outerwear else 0
    )

    color = calculate_color_score(outfit)
    formality = calculate_formality_score(outfit)
    weather = combine_weather(temp, rain)

    return {
        "color": color,
        "formality": formality,
        "weather": weather,
        "temperature": temp,
        "rain": rain,
        "outfit_warmth": warmth,
        "target_warmth": round(target_warmth(temperature), 2),
        "event": event_score,
        "style_penalty": penalty,
        "final": combine_scores(color, formality, weather, event_score, penalty),
    }


def calculate_outfit_score_v2(outfit, temperature, rain_probability, event):
    return score_breakdown(outfit, temperature, rain_probability, event)["final"]
