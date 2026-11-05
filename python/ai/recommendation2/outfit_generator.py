from itertools import product


def generate_outfits(clothing_items):
    """
    Generate possible outfit combinations from wardrobe items.

    Expected clothing item fields:
        clothing_item_id
        category_name
        item_name
        primary_color
        formality 
    """
    # add season later if needed

    tops = []
    bottoms = []
    dresses = []
    outerwear = []

    # Separate clothing items by category
    for item in clothing_items:
        category = item["category_name"].strip().lower()

        if category == "top":
            tops.append(item)

        elif category == "bottom":
            bottoms.append(item)

        elif category == "dress":
            dresses.append(item)

        elif category == "outerwear":
            outerwear.append(item)

    outfits = []

    # Generate Top + Bottom combinations
    for top, bottom in product(tops, bottoms):

        # Without outerwear
        outfits.append({
            "items": [top, bottom]
        })

        # With outerwear
        for jacket in outerwear:
            outfits.append({
                "items": [top, bottom, jacket]
            })

    # Generate Dress combinations
    for dress in dresses:

        # Without outerwear
        outfits.append({
            "items": [dress]
        })

        # With outerwear
        for jacket in outerwear:
            outfits.append({
                "items": [dress, jacket]
            })

    return outfits
