# Vestir

Vestir is a single-user Android application that recommends an outfit for the day. The user adds clothing photos, receives clothing classifications, and gets outfit recommendations based on their wardrobe, weather, and calendar events.

There is no login system. The current version stores data for one user in a local SQLite database.

## How it works

1. **Closet** - Add clothing using the camera or gallery. The embedded ML component classifies the image by category, sub-category, colour, and formality.
2. **Recommendations** - The app combines wardrobe items with cached weather and calendar information, then generates and ranks outfits.
3. **Calendar** - Import events from an iCal (`.ics`) link. Each event title is mapped to an event type (school, work, party, hangout or gym) and given a dress code, which the recommender uses to match outfit formality.
4. **History** - View outfits that the user has chosen.
5. **Settings** - Configure the weather location and calendar connection.

## Architecture

```
Qt/QML user interface
        |
        v
C++ backend
  |       |        |
  |       |        +-- Open-Meteo weather API
  |       +----------- iCal (.ics) calendar link
  +------------------- local SQLite database and photo storage
        |
        v
Embedded Python ML runtime
  +-- FashionCLIP image classification
  +-- event mapper and outfit recommendation
```

The ML code runs locally inside the application through `MlClient`. The C++ backend embeds Python and calls the Python classifier, event mapper and recommender directly, so no separate ML server is needed.

The application is built with Qt 6, C++20, Qt Quick/QML, and SQLite. The database is initialized from `src/database/schema.sql` when the application starts. Photos are stored in the application's local data directory.

## Project structure

```
.
├── ai                          # Classifier evaluation results (CSV files)
├── android                     # Android manifest, Gradle files, OpenSSL libraries
├── cmake
│   └── EmbedPython.cmake       # Embedded Python and FashionCLIP build setup
├── python                      # Embedded Python and ML code
│   ├── ai
│   │   ├── classifier          # Clothing image classification
│   │   └── recommendation2     # Event mapper and outfit ranking
│   ├── tests                   # Python/ML tests
│   └── export_fashionclip.py   # Exports FashionCLIP to ONNX assets
├── src
│   ├── backend                 # Database, repositories, services and MlClient
│   ├── database                # SQLite schema
│   ├── model                   # C++ QObject models exposed to QML
│   │   ├── helpers             # Camera and image-recognition helpers
│   │   └── pages               # Page models
│   └── view                    # QML pages and reusable components
├── tests                       # C++ backend tests
└── CMakeLists.txt              # Main CMake configuration
```

`python/vestir_server.py` is the old HTTP server and is no longer used by the app.

## Requirements

- Qt 6.10 or later (Quick, Quick Controls 2, Multimedia, Network)
- CMake 3.16 or later and a C++20 compiler (MinGW on Windows)
- Python 3.14 on your PATH. CMake uses it at configure time to create `python/.venv_build`.
- Android builds only: Android SDK and NDK 27.2.12479018

## Building and running

### Desktop

1. Open `CMakeLists.txt` in Qt Creator.
2. Select a desktop Qt kit, such as MinGW on Windows.
3. Configure, build and run `appVestir`.

### Android

1. Open `CMakeLists.txt` in Qt Creator.
2. Select the Android kit for **arm64-v8a**.
3. Connect an arm64 phone or start an arm64 emulator, then build and deploy.

Then `CMakeLists.txt`, run **Build > Run CMake**.

## Database

The schema is in `src/database/schema.sql`. The database stores clothing items, calendar events, weather records, recommendation batches, outfit recommendations, and app settings.

The schema is applied only when a new database is created. Existing databases are not migrated. After changing the schema during development, clear the app data (or delete the local database file) so it is recreated.

## Backend components

- `Database` - Opens SQLite, initializes the schema, and runs queries.
- `PhotoStore` - Stores temporary and wardrobe images locally.
- `ClothingRepository` - Saves and loads wardrobe items.
- `HistoryRepository` - Reads previously chosen outfits.
- `WeatherService` - Retrieves and caches weather data from Open-Meteo.
- `CalendarService` - Imports and stores events from an iCal link.
- `RecommendationService` - Combines wardrobe, weather, calendar, and ML results.
- `MlClient` - Calls the embedded Python classifier, event mapper and recommender.

`src/main.cpp` creates these objects, passes each one the others it depends on, and connects them to the QML page models.

## Python and ML

The ML code is in `python/`:

- `python/ai/classifier` classifies clothing photos into category, sub-category, colour and formality.
- `python/ai/recommendation2` contains the event mapper and the outfit ranker. The event mapper turns a calendar event title into school, work, party, hangout or gym.
- `python/export_fashionclip.py` exports FashionCLIP to ONNX files. The CMake build runs it.

See `python/README.md` for details, including the Python requirements files and the Python tests.

## Testing

Tests run automatically with GitHub Actions. They start on every push to `main` or `ci-tests` and on every pull request into `main`. To run one by hand, open the **Actions** tab on GitHub, pick the workflow on the left, then choose **Run workflow**. To see results, open the **Checks** tab of a pull request.

There are two workflows in `.github/workflows`:

- **C++ Tests** (`cpp-tests.yml`) tests the backend.
  - It builds the backend test programs in `tests/` and runs them with CTest.
  - The unit tests cover the database, repositories, weather, calendar, photo store and recommendation service. They use a fake `MlClient` (`tests/MlClientStub.cpp`), so they never start Python.
  - `test_mlclient_integration` runs the real embedded ML on the sample photos in `tests/test_images`.
- **Python Tests** (`python-tests.yml`) tests the ML code in `python/`.
  - It runs the fast classifier and recommender tests with pytest.
  - The slower tests with real model weights run every week, or when you start the workflow by hand with the real-model option ticked.
