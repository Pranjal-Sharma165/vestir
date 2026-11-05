from ai.recommendation.ranker import get_recommendations


clothing_items = [
    {
        "clothing_item_id": 1,
        "category_name": "Top",
        "item_name": "Black T-Shirt",
        "primary_color": "black",
        "formality": "casual"
    },
    {
        "clothing_item_id": 2,
        "category_name": "Top",
        "item_name": "White Shirt",
        "primary_color": "white",
        "formality": "smart-casual"
    },
    {
        "clothing_item_id": 3,
        "category_name": "Bottom",
        "item_name": "Blue Jeans",
        "primary_color": "blue",
        "formality": "casual"
    },
    {
        "clothing_item_id": 4,
        "category_name": "Bottom",
        "item_name": "Black Trousers",
        "primary_color": "black",
        "formality": "formal"
    },
    {
        "clothing_item_id": 5,
        "category_name": "Dress",
        "item_name": "Red Dress",
        "primary_color": "red",
        "formality": "formal"
    },
    {
        "clothing_item_id": 6,
        "category_name": "Outerwear",
        "item_name": "Beige Jacket",
        "primary_color": "beige",
        "formality": "smart-casual"
    }
]


# Simulate outfits that have already been shown
excluded_outfits = {
    frozenset({1, 3, 6})
}


recommendations = get_recommendations(
    clothing_items=clothing_items,
    temperature=10,
    rain_probability=60,
    event="school",
    excluded_outfits=excluded_outfits,
    limit=5
)


print(f"Number of recommendations: {len(recommendations)}")

for index, outfit in enumerate(recommendations, start=1):
    item_ids = [
        item["clothing_item_id"]
        for item in outfit["items"]
    ]

    print(f"\nRank {index}")
    print(f"Score: {outfit['score']}")
    print(f"Item IDs: {item_ids}")