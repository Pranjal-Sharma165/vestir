def is_valid_outfit(outfit):
    """
    Check whether an outfit follows our structural rules.
    """

    items = outfit["items"]

    # Get the categories of all clothing items
    categories = [
        item["category_name"].strip().lower()
        for item in items
    ]

    # An outfit cannot be empty
    if not categories:
        return False

    # Check for duplicate clothing items
    item_ids = [
        item["clothing_item_id"]
        for item in items
    ]

    if len(item_ids) != len(set(item_ids)):
        return False

    # Count the number of each category
    top_count = categories.count("top")
    bottom_count = categories.count("bottom")
    dress_count = categories.count("dress")
    outerwear_count = categories.count("outerwear")

    # Reject unsupported categories
    if len(categories) != (
        top_count + bottom_count + dress_count + outerwear_count
    ):
        return False

    # Allow at most one outerwear item
    if outerwear_count > 1:
        return False

    # Valid outfit: Top + Bottom
    if top_count == 1 and bottom_count == 1:
        return dress_count == 0

    # Valid outfit: Dress
    if dress_count == 1:
        return top_count == 0 and bottom_count == 0

    return False


def filter_valid_outfits(outfits):
    """
    Return only structurally valid outfits.
    """

    return [
        outfit
        for outfit in outfits
        if is_valid_outfit(outfit)
    ]
