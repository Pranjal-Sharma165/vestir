-- Vestir default data for SQLite
-- Run this after schema.sql.
PRAGMA foreign_keys = ON;

-- Insert the allowed clothing categories.
INSERT OR IGNORE INTO clothing_categories (category_name)
VALUES
    ('Top'),
    ('Bottom'),
    ('Dress'),
    ('Outwear');

-- Insert the allowed style preferences.
INSERT OR IGNORE INTO style_preferences (style_name)
VALUES
    ('Casual'),
    ('Sporty'),
    ('Smart-Casual'),
    ('Formal');
