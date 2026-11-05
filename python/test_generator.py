from ai.recommendation.outfit_generator import generate_outfits

clothing_items = [
    {"clothing_item_id": 1, "category_name": "Top"},
    {"clothing_item_id": 2, "category_name": "Top"},
    {"clothing_item_id": 3, "category_name": "Bottom"},
    {"clothing_item_id": 4, "category_name": "Bottom"},
    {"clothing_item_id": 5, "category_name": "Dress"},
    {"clothing_item_id": 6, "category_name": "Outerwear"}
]

outfits = generate_outfits(clothing_items)

print("Total outfits:", len(outfits))

for outfit in outfits:
    print([
        item["clothing_item_id"]
        for item in outfit["items"]
    ])