-- Vestir SQLite schema
-- Run on a NEW/empty database file (the app does this on first launch).
-- There is no migration: after changing this file, delete vestir.db or reinstall.

PRAGMA foreign_keys = ON;

CREATE TABLE clothing_categories (
    category_id INTEGER PRIMARY KEY AUTOINCREMENT,
    category_name TEXT NOT NULL UNIQUE
        CHECK (category_name IN ('Top', 'Bottom', 'Dress', 'Outerwear'))
);

-- Clothing items look their category up by name, so these rows must exist.
INSERT INTO clothing_categories (category_name)
VALUES ('Top'), ('Bottom'), ('Dress'), ('Outerwear');

CREATE TABLE weather_records (
    weather_id INTEGER PRIMARY KEY AUTOINCREMENT,
    location TEXT NOT NULL,
    forecast_time TEXT NOT NULL,
    temperature_c REAL,
    min_temp_c REAL,
    max_temp_c REAL,
    feels_like_c REAL,
    cloudiness TEXT,
    weather_code INTEGER,
    uv_index REAL CHECK (uv_index IS NULL OR uv_index >= 0),
    precipitation_probability INTEGER
        CHECK (precipitation_probability IS NULL OR precipitation_probability BETWEEN 0 AND 100),
    precipitation_mm REAL,
    humidity INTEGER CHECK (humidity IS NULL OR humidity BETWEEN 0 AND 100),
    wind_speed_kmh REAL,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (location, forecast_time)
);

CREATE TABLE clothing_items (
    clothing_item_id INTEGER PRIMARY KEY AUTOINCREMENT,
    category_id INTEGER,
    item_name TEXT,
    image_path TEXT NOT NULL,
    -- Values below match the ML classifier labels exactly (ai/classifier2/*).
    -- half-sleeve jacket is only used by the older classifier; kept for it.
    sub_category TEXT
        CHECK (sub_category IS NULL OR sub_category IN (
            't-shirt', 'blouse', 'sweater', 'hoodie', 'shirt', 'tank top',
            'jeans', 'trousers', 'shorts', 'skirt', 'leggings',
            'jacket', 'coat', 'blazer', 'parka', 'cardigan', 'vest', 'half-sleeve jacket',
            'dress', 'jumpsuit', 'romper')),
    primary_color TEXT
        CHECK (primary_color IS NULL OR primary_color IN (
            'black', 'white', 'gray', 'charcoal', 'brown',
            'beige', 'khaki', 'cream', 'navy blue', 'blue',
            'light blue', 'green', 'olive green', 'red', 'pink',
            'purple', 'yellow', 'orange', 'burgundy')),
    formality TEXT
        CHECK (formality IS NULL OR formality IN ('casual', 'smart-casual', 'formal', 'sporty')),
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (category_id) REFERENCES clothing_categories(category_id) ON DELETE SET NULL
);

CREATE INDEX idx_clothing_category ON clothing_items(category_id);
CREATE INDEX idx_clothing_color ON clothing_items(primary_color);

CREATE TABLE calendar_events (
    event_id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_name TEXT NOT NULL,
    event_type TEXT,
    location TEXT,
    dress_code TEXT
        CHECK (dress_code IS NULL OR dress_code IN ('casual', 'smart-casual', 'formal', 'sporty')),
    start_time TEXT NOT NULL,
    end_time TEXT,
    notes TEXT,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
    google_event_id TEXT UNIQUE,
    calendar_source TEXT NOT NULL DEFAULT 'manual',
    CHECK (end_time IS NULL OR end_time >= start_time)
);

CREATE INDEX idx_events_start ON calendar_events(start_time);

-- One local database file represents one user, so there is no user_id column.

-- One batch = one "generate" press (top 5). Pressing "generate again" creates a new batch
-- for the same day; items already shown are excluded using outfit_items of earlier batches.
CREATE TABLE recommendation_batches (
    batch_id INTEGER PRIMARY KEY AUTOINCREMENT,
    event_id INTEGER,
    weather_id INTEGER,
    created_at TEXT DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (event_id) REFERENCES calendar_events(event_id) ON DELETE SET NULL,
    FOREIGN KEY (weather_id) REFERENCES weather_records(weather_id) ON DELETE SET NULL
);

CREATE TABLE outfit_recommendations (
    recommendation_id INTEGER PRIMARY KEY AUTOINCREMENT,
    batch_id INTEGER NOT NULL,
    rank INTEGER NOT NULL CHECK (rank >= 1),
    score REAL NOT NULL,
    status TEXT NOT NULL DEFAULT 'shown'
        CHECK (status IN ('shown', 'chosen', 'rejected')),
    recommended_at TEXT DEFAULT CURRENT_TIMESTAMP,
    UNIQUE (batch_id, rank),
    FOREIGN KEY (batch_id) REFERENCES recommendation_batches(batch_id) ON DELETE CASCADE
);

CREATE INDEX idx_recommendations_date ON outfit_recommendations(recommended_at);
CREATE INDEX idx_recommendations_batch ON outfit_recommendations(batch_id);

CREATE TABLE outfit_items (
    recommendation_id INTEGER NOT NULL,
    clothing_item_id INTEGER NOT NULL,
    PRIMARY KEY (recommendation_id, clothing_item_id),
    FOREIGN KEY (recommendation_id) REFERENCES outfit_recommendations(recommendation_id) ON DELETE CASCADE,
    FOREIGN KEY (clothing_item_id) REFERENCES clothing_items(clothing_item_id) ON DELETE CASCADE
);

-- Simple key/value settings, e.g. 'ics_url' (calendar link).
CREATE TABLE IF NOT EXISTS app_settings (
    key TEXT PRIMARY KEY,
    value TEXT
);

-- Keep updated_at current when an item or calendar event is edited.
CREATE TRIGGER update_clothing_items_updated_at
AFTER UPDATE ON clothing_items
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE clothing_items
    SET updated_at = CURRENT_TIMESTAMP
    WHERE clothing_item_id = NEW.clothing_item_id;
END;

CREATE TRIGGER update_calendar_events_updated_at
AFTER UPDATE ON calendar_events
FOR EACH ROW
WHEN NEW.updated_at = OLD.updated_at
BEGIN
    UPDATE calendar_events
    SET updated_at = CURRENT_TIMESTAMP
    WHERE event_id = NEW.event_id;
END;
