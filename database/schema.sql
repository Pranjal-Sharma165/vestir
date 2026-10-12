-- Vestir database schema
-- WARNING: This resets the entire database and deletes all existing data.

DROP DATABASE IF EXISTS vestir_db;
CREATE DATABASE vestir_db
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_0900_ai_ci;

USE vestir_db;

-- 1. Users
CREATE TABLE users (
    user_id INT NOT NULL AUTO_INCREMENT,
    username VARCHAR(50) NOT NULL,
    email VARCHAR(255) NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (user_id),
    UNIQUE KEY uq_users_username (username),
    UNIQUE KEY uq_users_email (email)
) ENGINE=InnoDB;

-- 2. Clothing categories
CREATE TABLE clothing_categories (
    category_id INT NOT NULL AUTO_INCREMENT,
    category_name VARCHAR(50) NOT NULL,
    PRIMARY KEY (category_id),
    UNIQUE KEY uq_category_name (category_name)
) ENGINE=InnoDB;

-- 3. Style preferences
CREATE TABLE style_preferences (
    style_id INT NOT NULL AUTO_INCREMENT,
    style_name VARCHAR(50) NOT NULL,
    PRIMARY KEY (style_id),
    UNIQUE KEY uq_style_name (style_name)
) ENGINE=InnoDB;

-- 4. Weather records
CREATE TABLE weather_records (
    weather_id INT NOT NULL AUTO_INCREMENT,
    location VARCHAR(255) NOT NULL,
    forecast_time DATETIME NOT NULL,
    temperature_c DECIMAL(5,2) DEFAULT NULL,
    feels_like_c DECIMAL(5,2) DEFAULT NULL,
    weather_code SMALLINT UNSIGNED DEFAULT NULL,
    uv_index DECIMAL(4,1) DEFAULT NULL,
    precipitation_probability TINYINT UNSIGNED DEFAULT NULL,
    precipitation_mm DECIMAL(6,2) DEFAULT NULL,
    humidity TINYINT UNSIGNED DEFAULT NULL,
    wind_speed_kmh DECIMAL(6,2) DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (weather_id),
    UNIQUE KEY uq_weather_location_time (location, forecast_time),
    CONSTRAINT chk_weather_uv
        CHECK (uv_index IS NULL OR uv_index >= 0),
    CONSTRAINT chk_weather_precip_probability
        CHECK (
            precipitation_probability IS NULL
            OR precipitation_probability BETWEEN 0 AND 100
        ),
    CONSTRAINT chk_weather_humidity
        CHECK (humidity IS NULL OR humidity BETWEEN 0 AND 100)
) ENGINE=InnoDB;

-- 5. Clothing items
CREATE TABLE clothing_items (
    clothing_item_id INT NOT NULL AUTO_INCREMENT,
    user_id INT NOT NULL,
    category_id INT DEFAULT NULL,
    item_name VARCHAR(100) DEFAULT NULL,
    image_path VARCHAR(500) NOT NULL,
    primary_color VARCHAR(30) DEFAULT NULL,
    formality ENUM(
        'casual',
        'smart-casual',
        'formal',
        'sporty'
    ) DEFAULT NULL,
    season ENUM(
        'spring',
        'summer',
        'autumn',
        'winter',
        'all-season'
    ) DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (clothing_item_id),
    KEY idx_clothing_user (user_id),
    KEY idx_clothing_category (category_id),
    KEY idx_clothing_color (primary_color),
    CONSTRAINT fk_clothing_user
        FOREIGN KEY (user_id)
        REFERENCES users (user_id)
        ON DELETE CASCADE,
    CONSTRAINT fk_clothing_category
        FOREIGN KEY (category_id)
        REFERENCES clothing_categories (category_id)
        ON DELETE SET NULL
) ENGINE=InnoDB;

-- 6. Calendar events
CREATE TABLE calendar_events (
    event_id INT NOT NULL AUTO_INCREMENT,
    user_id INT NOT NULL,
    event_name VARCHAR(100) NOT NULL,
    event_type VARCHAR(50) DEFAULT NULL,
    location VARCHAR(255) DEFAULT NULL,
    dress_code ENUM(
        'casual',
        'smart-casual',
        'formal',
        'sporty'
    ) DEFAULT NULL,
    inferred_dress_code ENUM(
        'casual',
        'smart-casual',
        'formal',
        'sporty'
    ) DEFAULT NULL,
    dress_code_confirmed BOOLEAN NOT NULL DEFAULT FALSE,
    start_time DATETIME NOT NULL,
    end_time DATETIME DEFAULT NULL,
    notes TEXT DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        ON UPDATE CURRENT_TIMESTAMP,
    google_event_id VARCHAR(255) DEFAULT NULL,
    calendar_source VARCHAR(20) NOT NULL DEFAULT 'manual',
    PRIMARY KEY (event_id),
    UNIQUE KEY uq_google_calendar_event (user_id, google_event_id),
    KEY idx_events_user_start (user_id, start_time),
    CONSTRAINT fk_events_user
        FOREIGN KEY (user_id)
        REFERENCES users (user_id)
        ON DELETE CASCADE,
    CONSTRAINT chk_event_times
        CHECK (end_time IS NULL OR end_time >= start_time)
) ENGINE=InnoDB;

-- 7. User style preferences
CREATE TABLE user_style_preferences (
    user_id INT NOT NULL,
    style_id INT NOT NULL,
    preference_strength TINYINT UNSIGNED NOT NULL DEFAULT 3,
    PRIMARY KEY (user_id, style_id),
    KEY idx_user_style_id (style_id),
    CONSTRAINT fk_user_style_user
        FOREIGN KEY (user_id)
        REFERENCES users (user_id)
        ON DELETE CASCADE,
    CONSTRAINT fk_user_style_style
        FOREIGN KEY (style_id)
        REFERENCES style_preferences (style_id)
        ON DELETE CASCADE,
    CONSTRAINT chk_preference_strength
        CHECK (preference_strength BETWEEN 1 AND 5)
) ENGINE=InnoDB;

-- 8. Outfit recommendations
CREATE TABLE outfit_recommendations (
    recommendation_id INT NOT NULL AUTO_INCREMENT,
    user_id INT NOT NULL,
    event_id INT DEFAULT NULL,
    weather_id INT DEFAULT NULL,
    recommendation_reason TEXT DEFAULT NULL,
    recommended_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (recommendation_id),
    UNIQUE KEY uq_recommendation_user (recommendation_id, user_id),
    KEY idx_recommendations_event (event_id),
    KEY idx_recommendations_weather (weather_id),
    KEY idx_recommendations_user_date (user_id, recommended_at),
    CONSTRAINT fk_recommendations_user
        FOREIGN KEY (user_id)
        REFERENCES users (user_id)
        ON DELETE CASCADE,
    CONSTRAINT fk_recommendations_event
        FOREIGN KEY (event_id)
        REFERENCES calendar_events (event_id)
        ON DELETE SET NULL,
    CONSTRAINT fk_recommendations_weather
        FOREIGN KEY (weather_id)
        REFERENCES weather_records (weather_id)
        ON DELETE SET NULL
) ENGINE=InnoDB;

-- 9. Items belonging to each outfit recommendation
CREATE TABLE outfit_items (
    recommendation_id INT NOT NULL,
    clothing_item_id INT NOT NULL,
    PRIMARY KEY (recommendation_id, clothing_item_id),
    KEY idx_outfit_items_clothing (clothing_item_id),
    CONSTRAINT fk_outfit_items_recommendation
        FOREIGN KEY (recommendation_id)
        REFERENCES outfit_recommendations (recommendation_id)
        ON DELETE CASCADE,
    CONSTRAINT fk_outfit_items_clothing
        FOREIGN KEY (clothing_item_id)
        REFERENCES clothing_items (clothing_item_id)
        ON DELETE CASCADE
) ENGINE=InnoDB;

-- 10. Outfit feedback
CREATE TABLE outfit_feedback (
    feedback_id INT NOT NULL AUTO_INCREMENT,
    recommendation_id INT NOT NULL,
    user_id INT NOT NULL,
    liked BOOLEAN NOT NULL,
    feedback_comment TEXT DEFAULT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (feedback_id),
    UNIQUE KEY uq_feedback_user_recommendation (
        recommendation_id, user_id
    ),
    KEY idx_feedback_user (user_id),
    CONSTRAINT fk_feedback_recommendation_user
        FOREIGN KEY (recommendation_id, user_id)
        REFERENCES outfit_recommendations (recommendation_id, user_id)
        ON DELETE CASCADE,
    CONSTRAINT fk_feedback_user
        FOREIGN KEY (user_id)
        REFERENCES users (user_id)
        ON DELETE CASCADE
) ENGINE=InnoDB;
