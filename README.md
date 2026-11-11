# Vestir

Vestir is a single-user Android app that suggests the best outfit for the day. You upload photos of your clothes, the app classifies them, and it then ranks outfits using your closet, the weather and your calendar events.

## How it works

1. **Closet** – add clothes by camera or gallery. The ML server classifies each photo (category, type, colour, formality).
2. **Outfits** – the app gathers the weather and today's calendar events, asks the ML server for ranked outfits, and shows them one at a time. Choose one, or reject it to see the next. "Start over" generates a fresh batch.
3. **Calendar** – import events from an iCal (`.ics`) link. Each event can have its own outfit.
4. **History** – past chosen outfits.
5. **Settings** – city for the weather, calendar link, accent options.

There is no login: the database holds one user.

## Architecture

```
Qt/QML app (C++ backend, SQLite)  --HTTP-->  Python ML server (FastAPI)
        |
        +-- weather API, iCal calendar link
```

- The app is a Qt Quick (Qt 6) application written in C++ and QML.
- Data is stored in a local SQLite database, `vestir.db`. It is created from `src/database/schema.sql` the first time the app starts. The schema also inserts the four clothing categories (Top, Bottom, Dress, Outerwear).
- Photos are stored in a local folder on the device.
- The Python ML server does the image classification and outfit ranking. The app talks to it over HTTP (`MlClient`).

## Project structure

```
├── python              # ML server (FastAPI) and the ML team's code
│   ├── vestir_server.py   # Entry point: uvicorn vestir_server:app
│   └── ai              # Classifier and recommendation logic
└── src                 # Source for the Qt app
    ├── database        # schema.sql: creates the database tables and categories
    ├── model           # QObject classes used by QML (Model-View-Delegate)
    │   └── pages       # Backing model for each page
    ├── backend         # Database access, business services and ML client
    └── view            # QML presentation layer
        ├── components  # Shared QML components
        └── pages       # One folder per page
```

## Requirements

- Qt 6.12 with the Android kit (x86_64 for the emulator, arm64 for phones), plus the desktop MinGW kit if you want to run on the PC
- Android SDK and NDK 27.2.12479018
- Qt Creator (recommended)
- Python 3.10 with the packages in `python/requirements.txt`

## Running the ML server

From the `python` folder:

```
pip install -r requirements.txt
python -m uvicorn vestir_server:app --host 0.0.0.0 --port 8001
```

`--host 0.0.0.0` lets both the PC and the emulator reach it. Check that `http://127.0.0.1:8001/health` returns 200.

On Windows, allow inbound TCP port 8001 through the firewall (for example a rule named "Vestir ML 8001").

The app needs this server running to classify photos and to generate outfits. Without it, the Outfits page shows "Couldn't get outfits" with a **Try again** button.

## Server address

| Where the app runs | Default address |
| --- | --- |
| Android emulator | `http://10.0.2.2:8001` (the emulator's name for the host PC) |
| Desktop | `http://127.0.0.1:8001` |

Set the `VESTIR_ML_URL` environment variable to override it. A real phone needs the PC's LAN address (for example `http://192.168.1.20:8001`), and the phone must be on the same network.

## Building and running

1. Open `CMakeLists.txt` in Qt Creator and select the Android x86_64 kit.
2. Start an emulator (for example `Medium_Phone_API_37.0`).
3. Start the ML server (above), then build and run.

For a desktop run, select the MinGW kit instead. After adding or removing files, or editing `CMakeLists.txt`, run **Build > Run CMake**.

## Database changes

The schema runs only when `vestir.db` does not exist. After changing `schema.sql`, uninstall the app (or clear its data) so the database is recreated. Existing databases are not migrated.

## Notes for developers

- The ML code under `python/ai` belongs to the ML team.
- Weather and calendar need internet access. Photo classification and outfit ranking need the ML server to be reachable.
