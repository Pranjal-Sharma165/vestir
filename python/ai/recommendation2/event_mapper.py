import difflib
import math
import re
import unicodedata
from dataclasses import dataclass

from .scoring import EVENT_COMPATIBILITY

DEFAULT_EVENT = "hangout"   # the most neutral of the five

# Order matters only for ties: earlier events win.
EVENT_KEYWORDS = {
    "school": [
        "school", "college", "university", "uni", "class", "lecture", "lab",
        "seminar", "tutorial", "exam", "midterm", "finals", "final exam",
        "quiz", "study", "studying", "study group", "homework", "assignment",
        "campus", "professor", "prof", "course", "semester", "office hours",
        "group project", "thesis", "capstone", "tutor", "orientation",
    ],
    "work": [
        "work", "office", "meeting", "mtg", "interview", "client",
        "conference", "stand up", "shift", "business", "deadline",
        "networking", "internship", "one on one", "career fair", "boss",
        "manager", "sync", "job", "co op",
    ],
    "party": [
        "party", "birthday", "wedding", "dinner", "date night", "first date",
        "night out", "nightclub", "pub", "celebration", "prom", "gala",
        "reception", "anniversary", "graduation", "concert", "drinks",
        "cocktail", "festival", "karaoke", "bridal shower", "baby shower",
        "housewarming", "reunion", "formal",
    ],
    "hangout": [
        "hangout", "hang out", "lunch", "brunch", "coffee", "movie",
        "shopping", "mall", "picnic", "walk", "park", "friends", "game night",
        "beach", "cafe", "errands", "catch up", "chill", "board games",
        "video games", "sleepover", "bbq", "barbecue", "arcade", "bowling",
        "family", "visit",
    ],
    "gym": [
        "gym", "workout", "work out", "exercise", "run", "running", "jog",
        "jogging", "yoga", "pilates", "training", "practice", "swim",
        "swimming", "hike", "hiking", "fitness", "cardio", "lifting", "lift",
        "crossfit", "basketball", "soccer", "football", "tennis", "badminton",
        "volleyball", "sport", "spin", "climbing", "boxing", "leg day",
        "dance class", "yoga class", "spin class", "pilates class",
        "fitness class", "boxing class", "martial arts", "5k", "marathon",
    ],
}

# Used only by the optional embedding fallback. "other" catches titles that
# should NOT be forced into one of the five events (they get the default).
SEMANTIC_PROTOTYPES = {
    "school": [
        "a class or lecture at school or college",
        "a study session at the library or campus",
        "an exam, assignment or project for a university course",
    ],
    "work": [
        "a business meeting at the office",
        "a job interview or client call",
        "a professional work event or conference",
    ],
    "party": [
        "a party or celebration with dressy clothes",
        "a wedding, birthday dinner or night out",
    ],
    "hangout": [
        "a casual outing with friends",
        "coffee, lunch or a movie with friends",
        "relaxing, shopping or walking around",
    ],
    "gym": [
        "a workout or gym session",
        "exercise, running, yoga or sports practice",
    ],
    "other": [
        "a medical or dentist appointment",
        "a flight, trip or travel reminder",
        "a reminder, errand or task with no dress code",
    ],
}

PHRASE_WEIGHT = 2.0
WORD_WEIGHT = 1.0
FUZZY_WEIGHT = 0.5
FUZZY_CUTOFF = 0.88


@dataclass(frozen=True)
class EventMatch:
    event: str            # always one of the recommender's events
    confidence: float     # 0.0 (guessed / default) to 1.0
    method: str           # learned | keyword | fuzzy | semantic | default | empty
    matched: tuple = ()   # the keywords or prototype that decided it

# make the strings normalized for compariso
def normalize(text):
    text = unicodedata.normalize("NFKD", str(text or ""))
    text = text.encode("ascii", "ignore").decode("ascii").lower()
    return " ".join(re.sub(r"[^a-z0-9]+", " ", text).split())


def _forms(token):
    """The token plus simple singular forms: classes -> class, movies -> movie."""
    forms = {token}
    if len(token) > 3 and token.endswith("s") and not token.endswith("ss"):
        forms.add(token[:-1])
        if token.endswith("es"):
            forms.add(token[:-2])
    return forms


def _to_lists(vectors):
    if hasattr(vectors, "tolist"):          # torch / numpy
        vectors = vectors.tolist()
    return [[float(x) for x in v] for v in vectors]


def _cosine(a, b):
    dot = sum(x * y for x, y in zip(a, b))
    na = math.sqrt(sum(x * x for x in a))
    nb = math.sqrt(sum(y * y for y in b))
    return dot / (na * nb) if na and nb else 0.0


class EventMapper:
    def __init__(
        self,
        keywords=None,
        default_event=DEFAULT_EVENT,
        embed_fn=None,
        semantic_min_confidence=0.5,
        semantic_scale=100.0,
    ):
        """
        embed_fn: optional, list[str] -> one vector per string, e.g.
            shared_model.encode_text  (your FashionCLIPModel). Only used when
            no keyword matches. Its quality on real calendar titles is
            something to check on your own data.
        """
        keywords = keywords or EVENT_KEYWORDS
        unknown = set(keywords) - set(EVENT_COMPATIBILITY)
        if unknown:
            raise ValueError(f"Events not known to the recommender: {sorted(unknown)}")
        if default_event not in EVENT_COMPATIBILITY:
            raise ValueError(f"Unknown default event: {default_event}")

        self.default_event = default_event
        self.embed_fn = embed_fn
        self.semantic_min_confidence = semantic_min_confidence
        self.semantic_scale = semantic_scale
        self.learned = {}

        self._order = list(keywords)
        self._phrases = []     # (" padded phrase ", event, original)
        self._words = {}       # word -> [events]
        for event, words in keywords.items():
            for raw in words:
                kw = normalize(raw)
                if " " in kw:
                    self._phrases.append((f" {kw} ", event, kw))
                else:
                    self._words.setdefault(kw, []).append(event)
        self._proto_vecs = None

    def learn(self, title, event):
        if event not in EVENT_COMPATIBILITY:
            raise ValueError(f"Unknown event: {event}")
        self.learned[normalize(title)] = event

    def map(self, title):
        norm = normalize(title)
        if not norm:
            return EventMatch(self.default_event, 0.0, "empty")

        if norm in self.learned:
            return EventMatch(self.learned[norm], 1.0, "learned", (norm,))

        match = self._by_keywords(norm)
        if match:
            return match

        if self.embed_fn is not None:
            match = self._by_embedding(norm)
            if match:
                return match

        return EventMatch(self.default_event, 0.0, "default")

    def _by_keywords(self, norm):
        padded = f" {norm} "
        scores, matched, exact = {}, [], False

        def add(event, weight, label):
            scores[event] = scores.get(event, 0.0) + weight
            matched.append(label)

        for phrase, event, original in self._phrases:
            if phrase in padded:
                add(event, PHRASE_WEIGHT, original)
                exact = True

        unmatched = []
        for token in norm.split():
            hit = False
            for candidate in _forms(token):
                for event in self._words.get(candidate, ()):
                    add(event, WORD_WEIGHT, candidate)
                    hit = exact = True
            if not hit and len(token) >= 4:
                unmatched.append(token)

        for token in unmatched:       # typo tolerance, half weight
            close = difflib.get_close_matches(
                token, self._words, n=1, cutoff=FUZZY_CUTOFF
            )
            for word in close:
                for event in self._words[word]:
                    add(event, FUZZY_WEIGHT, word)

        if not scores:
            return None

        best = max(self._order, key=lambda e: (scores.get(e, 0.0), -self._order.index(e)))
        confidence = round(scores[best] / sum(scores.values()), 2)
        return EventMatch(
            best, confidence, "keyword" if exact else "fuzzy", tuple(matched)
        )

    def _by_embedding(self, norm):
        if self._proto_vecs is None:
            labels, texts = [], []
            for event, protos in SEMANTIC_PROTOTYPES.items():
                for text in protos:
                    labels.append(event)
                    texts.append(text)
            self._proto_labels = labels
            self._proto_texts = texts
            self._proto_vecs = _to_lists(self.embed_fn(texts))

        vec = _to_lists(self.embed_fn([norm]))[0]
        best_sim = {}
        best_text = {}
        for label, text, proto in zip(
            self._proto_labels, self._proto_texts, self._proto_vecs
        ):
            sim = _cosine(vec, proto)
            if sim > best_sim.get(label, -2.0):
                best_sim[label], best_text[label] = sim, text

        logits = {k: self.semantic_scale * v for k, v in best_sim.items()}
        top = max(logits.values())
        exps = {k: math.exp(v - top) for k, v in logits.items()}
        total = sum(exps.values())
        label = max(exps, key=exps.get)
        confidence = exps[label] / total

        if confidence < self.semantic_min_confidence or label == "other":
            return None
        return EventMatch(label, round(confidence, 2), "semantic", (best_text[label],))


_default_mapper = EventMapper()


def map_event(title):
    return _default_mapper.map(title)