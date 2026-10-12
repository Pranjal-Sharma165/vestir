-- Vestir default data
USE vestir_db;

-- Insert default clothing categories
INSERT INTO clothing_categories (category_name)
VALUES
    ('Shirt'),
    ('T-Shirt'),
    ('Pants'),
    ('Shorts'),
    ('Jacket'),
    ('Coat'),
    ('Dress'),
    ('Skirt'),
    ('Shoes'),
    ('Sweater'),
    ('Hoodie'),
    ('Suit'),
    ('Sportwear'),
    ('Other');

-- Insert default style preferences
INSERT INTO style_preferences (style_name)
VALUES
    ('Casual'),
    ('Minimalist'),
    ('Streetwear'),
    ('Classic'),
    ('Chic'),
    ('Bohemian'),
    ('Preppy'),
    ('Sporty'),
    ('Formal');
