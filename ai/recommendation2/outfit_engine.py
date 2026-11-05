from functools import lru_cache
from heapq import nlargest

from . import garment_traits as traits
from .scoring import (
    get_color_pair_score,
    get_formality_pair_score,
)
from .scoring_v2 import (
    combine_scores,
    combine_weather,
    item_event_score,
    rain_score,
    temperature_score,
)

# There are only ~19 colours and 4 formalities, so these caches are tiny.
_color_pair = lru_cache(maxsize=None)(get_color_pair_score)
_formality_pair = lru_cache(maxsize=None)(get_formality_pair_score)


def _pair_matrix(left, right, field, pair_fn):
    return [[pair_fn(a[field], b[field]) for b in right] for a in left]


def _key(*items):
    return frozenset(item["clothing_item_id"] for item in items)


def _prepare(items, event, keep):
    """Per-item event score and warmth; optionally keep only the best `keep`."""
    event_scores = [item_event_score(item, event) for item in items]

    if keep is not None and len(items) > keep:
        best = sorted(range(len(items)), key=lambda i: -event_scores[i])[:keep]
        best.sort()  # restore original order so ties behave predictably
        items = [items[i] for i in best]
        event_scores = [event_scores[i] for i in best]

    warmths = [traits.get_warmth(item) for item in items]
    return items, event_scores, warmths


def iter_scored_outfits(
    clothing_items,
    temperature,
    rain_probability,
    event,
    excluded_outfits=None,
    max_per_category=None,
):
    buckets = {"top": [], "bottom": [], "dress": [], "outerwear": []}
    for item in clothing_items:
        category = traits.get_category(item)
        if category in buckets:
            buckets[category].append(item)

    tops, ev_t, w_t = _prepare(buckets["top"], event, max_per_category)
    bottoms, ev_b, w_b = _prepare(buckets["bottom"], event, max_per_category)
    dresses, ev_d, w_d = _prepare(buckets["dress"], event, max_per_category)
    outer, ev_o, w_o = _prepare(buckets["outerwear"], event, max_per_category)

    # Warmth totals repeat a lot, so cache the temperature score per total.
    temp_for = lru_cache(maxsize=None)(
        lambda warmth: temperature_score(warmth, temperature)
    )
    rain_plain = rain_score(rain_probability, 0.0)
    rain_layered = [
        rain_score(rain_probability, traits.get_rain_protection(jacket))
        for jacket in outer
    ]

    excluded = excluded_outfits or None

    # ---- Top + Bottom (+ optional Outerwear) ----
    if tops and bottoms:
        c_tb = _pair_matrix(tops, bottoms, "primary_color", _color_pair)
        f_tb = _pair_matrix(tops, bottoms, "formality", _formality_pair)

        if outer:
            c_to = _pair_matrix(tops, outer, "primary_color", _color_pair)
            f_to = _pair_matrix(tops, outer, "formality", _formality_pair)
            c_bo = _pair_matrix(bottoms, outer, "primary_color", _color_pair)
            f_bo = _pair_matrix(bottoms, outer, "formality", _formality_pair)
            style_to = [
                [traits.get_top_outerwear_penalty(t, j) for j in outer]
                for t in tops
            ]

        for ti, top in enumerate(tops):
            for bi, bottom in enumerate(bottoms):
                c = c_tb[ti][bi]
                f = f_tb[ti][bi]
                e = round((ev_t[ti] + ev_b[bi]) / 2, 2)
                warmth = w_t[ti] + w_b[bi]

                if not excluded or _key(top, bottom) not in excluded:
                    weather = combine_weather(temp_for(warmth), rain_plain)
                    yield combine_scores(c, f, weather, e), (top, bottom)

                for oi, jacket in enumerate(outer):
                    if excluded and _key(top, bottom, jacket) in excluded:
                        continue

                    c3 = round((c + c_to[ti][oi] + c_bo[bi][oi]) / 3, 2)
                    f3 = round((f + f_to[ti][oi] + f_bo[bi][oi]) / 3, 2)
                    e3 = round((ev_t[ti] + ev_b[bi] + ev_o[oi]) / 3, 2)
                    weather = combine_weather(
                        temp_for(warmth + w_o[oi]), rain_layered[oi]
                    )

                    yield (
                        combine_scores(c3, f3, weather, e3, style_to[ti][oi]),
                        (top, bottom, jacket),
                    )

    # ---- Dress (+ optional Outerwear) ----
    if dresses:
        if outer:
            c_do = _pair_matrix(dresses, outer, "primary_color", _color_pair)
            f_do = _pair_matrix(dresses, outer, "formality", _formality_pair)

        for di, dress in enumerate(dresses):
            if not excluded or _key(dress) not in excluded:
                weather = combine_weather(temp_for(w_d[di]), rain_plain)
                yield (
                    combine_scores(100.0, 100.0, weather, round(ev_d[di], 2)),
                    (dress,),
                )

            for oi, jacket in enumerate(outer):
                if excluded and _key(dress, jacket) in excluded:
                    continue

                e2 = round((ev_d[di] + ev_o[oi]) / 2, 2)
                weather = combine_weather(
                    temp_for(w_d[di] + w_o[oi]), rain_layered[oi]
                )
                yield (
                    combine_scores(c_do[di][oi], f_do[di][oi], weather, e2),
                    (dress, jacket),
                )


def _score_of(scored):
    return scored[0]


def top_outfits(
    clothing_items,
    temperature,
    rain_probability,
    event,
    excluded_outfits=None,
    limit=None,
    max_per_category=None,
):
    """
    Best outfits, highest score first, as {"items": [...], "score": float}.

    limit=None returns every outfit (full sort); otherwise a bounded heap
    keeps memory flat no matter how many outfits exist.
    """
    scored = iter_scored_outfits(
        clothing_items,
        temperature,
        rain_probability,
        event,
        excluded_outfits=excluded_outfits,
        max_per_category=max_per_category,
    )

    if limit is None:
        ranked = sorted(scored, key=_score_of, reverse=True)
    else:
        ranked = nlargest(limit, scored, key=_score_of)

    return [
        {"items": list(items), "score": score}
        for score, items in ranked
    ]
