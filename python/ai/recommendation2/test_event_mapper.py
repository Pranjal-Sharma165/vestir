import math
import zlib

from .event_mapper import EventMapper, map_event, normalize
from .scoring import EVENT_COMPATIBILITY


def test_normalize():
    assert normalize("  Café – 5K Run!! ") == "cafe 5k run"
    assert normalize(None) == ""


def test_school_titles():
    for title in [
        "College lecture", "COMP 7082 Lab", "Midterm exam", "Study group",
        "university", "Office hours with Prof. Lee", "Classes", "School",
        "Final exam - Chemistry",
    ]:
        assert map_event(title).event == "school", title


def test_work_titles():
    for title in [
        "Team stand-up", "Interview with Google", "Client meeting",
        "Mtg w/ Sam", "Work", "Shift at the store", "Career fair",
    ]:
        assert map_event(title).event == "work", title


def test_party_titles():
    for title in [
        "Mom's birthday dinner", "Sarah's wedding", "Night out",
        "Graduation party", "Concert", "Housewarming",
    ]:
        assert map_event(title).event == "party", title


def test_hangout_titles():
    for title in [
        "Coffee with Sam", "Brunch", "Movie night", "Shopping at the mall",
        "Hang out with friends", "Café catch-up", "Picnic in the park",
    ]:
        assert map_event(title).event == "hangout", title


def test_gym_titles():
    for title in [
        "Gym", "Leg day", "5k run", "Yoga class", "Workout", "Work out",
        "Basketball practice", "Hiking trip", "Swimming",
    ]:
        assert map_event(title).event == "gym", title


def test_typos_still_map():
    assert map_event("Lecure").event == "school"
    assert map_event("Meting with boss").event == "work"
    assert map_event("Birthdy dinner").event == "party"
    assert map_event("Lecure").method == "fuzzy"


def test_unknown_title_gets_default_with_zero_confidence():
    m = map_event("Dentist appointment")
    assert (m.event, m.confidence, m.method) == ("hangout", 0.0, "default")
    assert map_event("").method == "empty"
    assert map_event(None).event == "hangout"


def test_ambiguous_title_has_lower_confidence_than_clear_one():
    clear = map_event("Midterm exam lecture")
    mixed = map_event("Lunch meeting")             # hangout vs work
    assert clear.confidence == 1.0
    assert mixed.confidence == 0.5
    assert mixed.event == "work"                   # ties go to the earlier event


def test_learned_mapping_wins():
    mapper = EventMapper()
    assert mapper.map("Dentist appointment").method == "default"
    mapper.learn("Dentist appointment", "work")
    m = mapper.map("  dentist APPOINTMENT ")
    assert (m.event, m.method, m.confidence) == ("work", "learned", 1.0)


def test_every_recommender_event_is_reachable():
    mapper = EventMapper()
    assert set(mapper._order) == set(EVENT_COMPATIBILITY)


def test_results_are_always_valid_events():
    for title in ["???", "12345", "x", "a very long title " * 20, "日本語"]:
        assert map_event(title).event in EVENT_COMPATIBILITY

def fake_embed(texts, dim=256):
    out = []
    for text in texts:
        vec = [0.0] * dim
        for word in normalize(text).split():
            vec[zlib.crc32(word.encode()) % dim] += 1.0
        norm = math.sqrt(sum(x * x for x in vec)) or 1.0
        out.append([x / norm for x in vec])
    return out


def test_semantic_fallback_maps_when_no_keyword_matches():
    mapper = EventMapper(embed_fn=fake_embed)
    m = mapper.map("Library session")           # no keyword, shares "library"
    assert (m.event, m.method) == ("school", "semantic")


def test_semantic_other_and_nonsense_fall_back_to_default():
    mapper = EventMapper(embed_fn=fake_embed)
    assert mapper.map("Dentist appointment").method == "default"   # "other" class
    assert mapper.map("zzz qqq").method == "default"


def test_keywords_beat_semantic():
    mapper = EventMapper(embed_fn=fake_embed)
    assert mapper.map("Coffee").method == "keyword"


if __name__ == "__main__":
    for name, fn in list(globals().items()):
        if name.startswith("test_") and callable(fn):
            fn()
            print("ok", name)