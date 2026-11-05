import time

from ai.recommendation2.ranker import get_recommendations
from ai.recommendation2.test_outfit_engine import make_wardrobe, reference_rank

SIZES = [
    ("12 items (your test set)", (5, 3, 2, 2), True),
    ("typical: 40/25/15/10", (40, 25, 15, 10), True),
    ("large: 100/60/30/30", (100, 60, 30, 30), True),
    ("huge: 200/120/50/50", (200, 120, 50, 50), False),
]


def timed(fn):
    start = time.perf_counter()
    result = fn()
    return result, time.perf_counter() - start


def main():
    args = (12, 30, "work")
    print(f"{'wardrobe':28} {'outfits':>9} {'old (s)':>9} {'new (s)':>9} {'new+prune (s)':>14}")
    for label, (t, b, d, o), run_old in SIZES:
        wardrobe = make_wardrobe(t, b, d, o, seed=42)
        n_outfits = t * b * (o + 1) + d * (o + 1)

        old_time = None
        if run_old:
            old, old_time = timed(lambda: reference_rank(wardrobe, *args)[:5])

        new, new_time = timed(lambda: get_recommendations(wardrobe, *args, limit=5))
        _, pruned_time = timed(
            lambda: get_recommendations(wardrobe, *args, limit=5, max_per_category=30)
        )

        if run_old:
            assert [o["score"] for o in old] == [o["score"] for o in new]

        old_txt = f"{old_time:9.3f}" if old_time is not None else "      n/a"
        print(f"{label:28} {n_outfits:9,} {old_txt} {new_time:9.3f} {pruned_time:14.3f}")


if __name__ == "__main__":
    main()
    