#ifndef PHOTOSTORE_H
#define PHOTOSTORE_H

#include <QString>

/*!
 * \brief Moves clothing photos between the temporary camera folder and the
 * permanent wardrobe folder.
 *
 * The camera writes every shot to <dataDir>/photos (temporary). Only photos the
 * user confirms are copied into <dataDir>/wardrobe by addPhoto().
 */
class PhotoStore {
public:
  /*!
   * Copies the photo at \a tempPath (a local path or file URL) into wardrobe/
   * under a unique name, then (if \a removeTemp) deletes the temporary file. Returns the stored
   * relative path (e.g. "wardrobe/IMG_xxx.jpg") for the database, or an empty
   * string on failure.
   */
  static QString addPhoto(const QString &tempPath, bool removeTemp = true);

  /*! Deletes a temporary photo (Cancel/Retake). Only touches photos/. */
  static void discardPhoto(const QString &tempPath);

  /*! Deletes every leftover temporary photo. Called at startup. */
  static void clearTempPhotos();

  /*! The temporary camera folder. */
  static QString tempDir();
};

#endif // PHOTOSTORE_H
