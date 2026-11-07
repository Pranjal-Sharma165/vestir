#ifndef DATABASE_H
#define DATABASE_H

#include <QSqlDatabase>
#include <QString>

/*!
 * \brief Owns the single-user SQLite database and the local wardrobe folder.
 *
 * On first run, initialize() creates the app data folder, creates vestir.db
 * from the embedded schema.sql and seed.sql. Later runs just open the existing file. Image paths stored in the
 * database are relative to the app data folder (e.g. "wardrobe/IMG_1.jpg").
 */
class Database {
public:
  /*!
   * Prepares the app data folder and opens the database. Pass a non-empty
   * \a dataDir to override the default location (used for testing).
   * Returns false and logs on failure.
   */
  static bool initialize(const QString &dataDir = {});

  /*! The shared connection. Only valid after a successful initialize(). */
  static QSqlDatabase connection();

  /*! Folder that holds vestir.db and wardrobe/. */
  static QString dataDir();

  /*! Folder for user and sample clothing photos. */
  static QString wardrobeDir();

  /*! Turns a stored relative image path into an absolute file path. */
  static QString absoluteImagePath(const QString &storedPath);

  /*! Turns an absolute image path inside dataDir() into its stored form. */
  static QString relativeImagePath(const QString &absolutePath);

private:
  static constexpr const char *kConnectionName = "vestir";
};

#endif // DATABASE_H
