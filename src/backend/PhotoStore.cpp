#include "PhotoStore.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <random>
#include <system_error>
#include <utility>

namespace {

// Short name for std::filesystem
namespace fs = std::filesystem;

// Creates a random hexadecimal string for unique filenames
std::string randomHex(std::size_t length) {
  static std::mt19937_64 generator(
      static_cast<std::mt19937_64::result_type>(
          std::chrono::steady_clock::now().time_since_epoch().count()));
  static constexpr char digits[] = "0123456789abcdef";

  std::string value;
  value.reserve(length);
  while (value.size() < length) {
    const std::uint64_t number = generator();
    for (int index = 0; index < 16 && value.size() < length; ++index)
      value.push_back(digits[(number >> (index * 4)) & 0x0f]);
  }
  return value;
}

// Converts a path to a canonical or absolute path for reliable comparison
fs::path canonicalOrAbsolute(const fs::path &path) {
  std::error_code error;
  const fs::path canonical = fs::weakly_canonical(path, error);
  if (!error)
    return canonical;

  error.clear();
  const fs::path absolute = fs::absolute(path, error);
  return error ? path : absolute;
}

// Checks whether path is located inside directory
bool isInside(const fs::path &directory, const fs::path &path) {
  const fs::path relative = canonicalOrAbsolute(path).lexically_relative(
      canonicalOrAbsolute(directory));
  return !relative.empty() && relative != "." && relative != ".." &&
        relative.begin() != relative.end() && *relative.begin() != "..";
}

} // namespace

PhotoStore::PhotoStore(std::filesystem::path dataDir)
    : m_dataDir(std::move(dataDir)) {}

std::filesystem::path PhotoStore::wardrobeDir() const {
  return m_dataDir / "wardrobe";
}

std::filesystem::path PhotoStore::tempDir() const {
  return m_dataDir / "photos";
}

std::filesystem::path PhotoStore::newTempPath(const std::string &extension) const {
  std::error_code error;
  fs::create_directories(tempDir(), error);
  if (error)
    return {};

  const std::string normalizedExtension =
      extension.empty() ? "jpg" : extension.front() == '.' ? extension.substr(1)
                                                            : extension;
  return tempDir() / (randomHex(32) + "." + normalizedExtension);
}

std::filesystem::path PhotoStore::addPhoto(
    const std::filesystem::path &tempPath, bool removeTemp) {
  std::error_code error;
  if (!fs::is_regular_file(tempPath, error) || error)
    return {};

  error.clear();
  fs::create_directories(wardrobeDir(), error);
  if (error)
    return {};

  std::string extension = tempPath.extension().string();
  if (!extension.empty() && extension.front() == '.')
    extension.erase(0, 1);
  std::transform(extension.begin(), extension.end(), extension.begin(),
                 [](unsigned char character) {
                   return static_cast<char>(std::tolower(character));
                 });
  if (extension.empty())
    extension = "jpg";

  const fs::path target = wardrobeDir() / ("IMG_" + randomHex(12) + "." +
                                           extension);
  error.clear();
  if (!fs::copy_file(tempPath, target, fs::copy_options::none, error) ||
      error)
    return {};

  if (removeTemp && isInside(tempDir(), tempPath))
    fs::remove(tempPath, error);
  return relativeImagePath(target);
}

void PhotoStore::discardPhoto(const std::filesystem::path &tempPath) {
  if (isInside(tempDir(), tempPath)) {
    std::error_code error;
    fs::remove(tempPath, error);
  }
}

void PhotoStore::clearTempPhotos() {
  std::error_code error;
  if (!fs::is_directory(tempDir(), error))
    return;

  for (const fs::directory_entry &entry :
       fs::directory_iterator(tempDir(), error)) {
    if (error)
      return;
    if (entry.is_regular_file(error))
      fs::remove(entry.path(), error);
    if (error)
      return;
  }
}

std::filesystem::path PhotoStore::absoluteImagePath(
    const std::filesystem::path &storedPath) const {
  if (storedPath.empty() || storedPath.is_absolute())
    return storedPath;
  return m_dataDir / storedPath;
}

std::filesystem::path PhotoStore::relativeImagePath(
    const std::filesystem::path &absolutePath) const {
  std::error_code error;
  const fs::path relative = fs::relative(absolutePath, m_dataDir, error);
  return error ? absolutePath.lexically_relative(m_dataDir) : relative;
}
