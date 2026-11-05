from .outfit_engine import top_outfits
from .scoring_v2 import score_breakdown


def get_outfit_key(outfit):
    return frozenset(
        item["clothing_item_id"]
        for item in outfit["items"]
    )


def rank_outfits(
    clothing_items,
    temperature,
    rain_probability,
    event,
    excluded_outfits=None,
    limit=None,
    max_per_category=None,
):
    """
    Rank outfits from best to worst.

    limit=None returns all outfits. Pass a limit when you only need the best
    few; it is much cheaper on large wardrobes.
    """
    return top_outfits(
        clothing_items,
        temperature,
        rain_probability,
        event,
        excluded_outfits=excluded_outfits,
        limit=limit,
        max_per_category=max_per_category,
    )


def select_top_outfits(ranked_outfits, limit=5):
    return ranked_outfits[:limit]


def diversify(candidates, limit, penalty):
    """
    Greedy re-ranking: each already-selected outfit lowers the score of
    candidates that reuse its items, by `penalty` points per shared item.
    """
    pool = list(candidates)
    selected = []
    selected_keys = []

    while pool and len(selected) < limit:
        def adjusted(outfit):
            key = get_outfit_key(outfit)
            shared = sum(len(key & other) for other in selected_keys)
            return outfit["score"] - penalty * shared

        best = max(pool, key=adjusted)  # first max wins, so ties stay stable
        selected.append(best)
        selected_keys.append(get_outfit_key(best))
        pool.remove(best)

    return selected


def get_recommendations(
    clothing_items,
    temperature,
    rain_probability,
    event,
    excluded_outfits=None,
    limit=5,
    diversity=0.0,
    max_per_category=None,
    include_breakdown=False,
):
    """
    diversity: 0 keeps the pure score order. A value like 3 means
        "lose 3 score points per item shared with an outfit already chosen",
        so the top results are not near-copies of each other.
    max_per_category: optional pruning for huge wardrobes (approximate).
    include_breakdown: add outfit["breakdown"] (colour, weather, event, ...)
        to each returned outfit, e.g. to show "why this outfit" in the app.
    """
    pool_size = limit if diversity <= 0 else max(limit * 10, 50)

    candidates = rank_outfits(
        clothing_items=clothing_items,
        temperature=temperature,
        rain_probability=rain_probability,
        event=event,
        excluded_outfits=excluded_outfits,
        limit=pool_size,
        max_per_category=max_per_category,
    )

    if diversity <= 0:
        chosen = select_top_outfits(candidates, limit)
    else:
        chosen = diversify(candidates, limit, diversity)

    if include_breakdown:
        for outfit in chosen:
            outfit["breakdown"] = score_breakdown(
                outfit, temperature, rain_probability, event
            )

    return chosen
