"""
Command-line entry point used by the C++ backend (src/repository/MlClient).

Maps raw calendar titles to the recommender's event types, so the Calendar
page can show a formality before any outfit was requested:

    stdin  : {"titles": ["Job interview", "Gym", ...]}
    stdout : {"types": {"Job interview": "work", "Gym": "gym", ...}}

It only calls ai.recommendation2.event_mapper.map_event.
"""
import json
import sys

from ai.recommendation2.event_mapper import map_event


def main():
    req = json.load(sys.stdin)
    types = {t: map_event(t).event for t in req.get("titles", [])}
    json.dump({"types": types}, sys.stdout)


if __name__ == "__main__":
    main()
