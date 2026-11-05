from ai.recommendation.outfit_generator import generate_outfits
from ai.recommendation.scoring import calculate_outfit_score

def rank_outfits(
    clothing_items,
    temperature,
    rain_probability,
    event,
    excluded_outfits=None
):
    outfits = generate_outfits(clothing_items)

    excluded_outfits = excluded_outfits or set()

    # Remove previously recommended outfits
    outfits = [
        outfit for outfit in outfits
        if get_outfit_key(outfit) not in excluded_outfits
    ]

    # Calculate scores
    for outfit in outfits:
        outfit["score"] = calculate_outfit_score(
            outfit,
            temperature,
            rain_probability,
            event
        )

    # Sort from highest to lowest
    outfits.sort(
        key=lambda outfit: outfit["score"],
        reverse=True
    )

    return outfits

def select_top_outfits(ranked_outfits, limit=5):
    return ranked_outfits[:limit]

def get_outfit_key(outfit):
    return frozenset(
        item["clothing_item_id"]
        for item in outfit["items"]
    )

def get_recommendations(
    clothing_items,
    temperature,
    rain_probability,
    event,
    excluded_outfits=None,
    limit=5
):
    ranked_outfits = rank_outfits(
        clothing_items=clothing_items,
        temperature=temperature,
        rain_probability=rain_probability,
        event=event,
        excluded_outfits=excluded_outfits
    )

    return select_top_outfits(ranked_outfits, limit)