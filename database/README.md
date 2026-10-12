# Vestir Database

## Overview

This folder contains the MySQL database schema, default data, and EER diagram for the Vestir project.

The database stores user accounts, clothing items, style preferences, calendar events, weather information, outfit recommendations, and user feedback.

## Files

- `schema.sql` — Creates the `vestir_db` database and its 10 tables.
- `seed.sql` — Inserts the default clothing categories and style preferences.
- `EER.png` — Entity Relationship Diagram showing the database tables and their relationships.

## Requirements

- MySQL Server 8.0 or later
- MySQL Workbench (recommended)

## Database Setup

### 1. Create the database and tables

Open `schema.sql` in MySQL Workbench and execute the entire script.

**Warning:** This script drops and recreates `vestir_db`. Any existing data in that database will be deleted.

### 2. Insert default data

After the schema script finishes successfully, open `seed.sql` and execute the entire script.

The seed script inserts:

- 14 clothing categories
- 9 style preferences

### 3. Verify the setup

Run the following SQL query:

```sql
USE vestir_db;

SHOW TABLES;

SELECT * FROM clothing_categories ORDER BY category_id;
SELECT * FROM style_preferences ORDER BY style_id;
```

You should see 10 tables, 14 clothing categories, and 9 style preferences.

## Database Tables

| Table | Purpose |
|---|---|
| `users` | Stores user account information |
| `clothing_categories` | Stores clothing category names |
| `clothing_items` | Stores each user's wardrobe items |
| `style_preferences` | Stores available style types |
| `user_style_preferences` | Connects users to their style preferences and preference strengths |
| `calendar_events` | Stores calendar events and dress-code information |
| `weather_records` | Stores weather forecast information |
| `outfit_recommendations` | Stores generated outfit recommendations |
| `outfit_items` | Connects outfit recommendations to clothing items |
| `outfit_feedback` | Stores users' feedback on recommendations |

## Notes

- Run `schema.sql` before `seed.sql`.
- The scripts are intended for local development and initial setup.
- Do not run the schema reset script if you need to keep existing database data.
- Do not commit database passwords, API keys, access tokens, or real user data to GitHub.