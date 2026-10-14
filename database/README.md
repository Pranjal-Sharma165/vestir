# Vestir Database

Vestir uses **SQLite** to store clothing information and support outfit recommendations. Each user has their own local database file.

## Tables

The database has 9 tables, grouped by purpose:

- **Clothing and Style Preferences:** Stores clothing items, clothing categories, and selected style preferences.
- **Outfit Recommendations and Feedback:** Stores suggested outfits, the clothing items in each outfit, and user feedback.
- **Calendar and Weather:** Stores calendar events and weather records to help generate outfit suggestions based on the user's plans and weather conditions.

See `vestir database diagram.png` for the database relationships.

## Files

- `schema.sql` — Creates the database tables and relationships.
- `seed.sql` — Adds default clothing categories and style preferences.
- `vestir.db` — The SQLite database file.
- `vestir database diagram.png` — Database relationship diagram.

## Setup

1. Open `vestir.db` using DB Browser for SQLite.
2. To create the tables from scratch, open the **Execute SQL** tab and run `schema.sql`.
3. Run `seed.sql` to insert the default categories and style preferences.
4. Click **Write Changes** to save.

**Note:** Running `schema.sql` resets the existing tables and deletes their data. Back up the database before running it again.