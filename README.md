# Vestir

## Project Structure
Vestir is a Qt Quick Android application. It makes calls to a Python subprocess to perform image recognition and outfit recommendation. The project structure is as follows:

```
├── python              # Root for source of Python subprocess for AI model
│   └── ai              # Logic for the different types of models
└── src                 # Source for the mobile build, i.e. the Vestir app
    ├── database        # SQL files for bootstrapping the database
    ├── model           # QObject-derived classes for use in the frontend (see Qt's Model-View-Delegate)
    │   └── pages       # Backing models for each page in the app
    ├── repository      # Repository layer to access the database
    └── view            # QML files, which define the presentation layer (see Qt's Model-View-Delegate)
        ├── components  # Common QML components used across many pages
        └── pages       # QML files specific to each page
```