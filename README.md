# Vestir

Outfit recommendations from your wardrobe, the weather and your calendar.

| Folder | Contents |
|---|---|
| `frontend/` | Qt 6 / QML app plus the C++ database layer (`src/backend`) |
| `ai-models/` | Python: classifier, recommender, `recommend_cli.py`, `calendar_sync.py`, `import_wardrobe.py` |
| `database/sqlite/` | `schema.sql`, `seed.sql`, template `vestir.db` |
| `wardrobe/` | Clothing photos. Paths in the DB are relative to the repo root |

## Run
1. `cd ai-models`, then `py -3.10 -m venv .venv`, activate it, and
   `pip install -r requirements-backend.txt`
   (plus `ai/classifier/requirements.txt` to import new photos).
2. Build `frontend` with CMake and Qt 6.12 (MinGW), then run `appVestir.exe`.
   On first run the app copies `database/sqlite/vestir.db` to
   `%APPDATA%\Vestir\Vestir\vestir.db`; delete that file to reset.
3. Add photos: put them in `wardrobe/`, then `python import_wardrobe.py`
   from `ai-models`.
4. Calendar: paste a Google Calendar "secret address in iCal format" link on the
   Calendar page. Keep that link private.

Set `VESTIR_PYTHON` to use a different Python, `VESTIR_DB` to use another database file.
