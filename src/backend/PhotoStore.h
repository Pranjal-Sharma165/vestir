#pragma once

#include <filesystem>
#include <string>

class PhotoStore {
public:
    explicit PhotoStore(std::filesystem::path dataDir);

    // Copy temp photo into wardrobe/, delete the temp file, and return stored relative path
    std::filesystem::path addPhoto(const std::filesystem::path &tempPath, bool removeTemp = true);
    // Delete temp photo
    void discardPhoto(const std::filesystem::path &tempPath);
    // Delete all temp photo
    void clearTempPhotos();

    // Return path of temp photo folder /photos
    std::filesystem::path tempDir() const;
    // Create temp folder if doesn't exist, return the unque file name
    std::filesystem::path newTempPath(const std::string &extension = "jpg") const;

    // convert database path to full device path
    std::filesystem::path absoluteImagePath(const std::filesystem::path &storedPath) const;
    // convert full device path to database path
    std::filesystem::path relativeImagePath(const std::filesystem::path &absolutePath) const;

private:
    // Private helper that returns dataDir/wardrobe
    std::filesystem::path wardrobeDir() const;

    std::filesystem::path m_dataDir;
};