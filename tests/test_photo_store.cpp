#include "PhotoStore.h"
#include "TestHelpers.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

struct Fixture {
  fs::path dir;
  PhotoStore store;

  explicit Fixture(const std::string &name)
      : dir(fs::temp_directory_path() / ("vestir_test_photo_" + name)),
        store(dir) {
    fs::remove_all(dir);
  }

  ~Fixture() { fs::remove_all(dir); }

  fs::path makeTemp(const std::string &extension = "jpg") {
    const fs::path path = store.newTempPath(extension);
    std::ofstream(path) << "fake image";
    return path;
  }
};

static void testNewTempPath() {
  Fixture f("temp");
  const fs::path a = f.store.newTempPath("png");
  const fs::path b = f.store.newTempPath(".png");
  check(fs::is_directory(f.store.tempDir()), "temp folder is created");
  check(a.extension() == ".png" && b.extension() == ".png",
        "extension is used with or without a dot");
  check(a != b, "each temp name is unique");
  check(f.store.newTempPath("").extension() == ".jpg",
        "empty extension becomes jpg");
}

static void testAddPhoto() {
  Fixture f("add");
  const fs::path temp = f.makeTemp("JPG");
  const fs::path stored = f.store.addPhoto(temp);

  check(!stored.empty() && stored.is_relative(),
        "addPhoto returns a relative path");
  check(stored.parent_path() == "wardrobe", "photo is stored in wardrobe");
  check(stored.extension() == ".jpg", "extension is lowercased");
  check(fs::exists(f.store.absoluteImagePath(stored)),
        "stored photo exists on disk");
  check(!fs::exists(temp), "temp photo is removed");
}

static void testAddPhotoKeepTemp() {
  Fixture f("keep");
  const fs::path temp = f.makeTemp();
  f.store.addPhoto(temp, false);
  check(fs::exists(temp), "temp photo is kept when removeTemp is false");
}

static void testAddMissingPhoto() {
  Fixture f("missing");
  check(f.store.addPhoto(f.dir / "nothing.jpg").empty(),
        "missing photo returns an empty path");
}

static void testDiscardOnlyInsideTemp() {
  Fixture f("discard");
  const fs::path temp = f.makeTemp();
  f.store.discardPhoto(temp);
  check(!fs::exists(temp), "discardPhoto removes a temp photo");

  const fs::path outside = f.dir / "outside.jpg";
  std::ofstream(outside) << "x";
  f.store.discardPhoto(outside);
  check(fs::exists(outside), "discardPhoto keeps files outside the temp folder");
}

static void testClearTempPhotos() {
  Fixture f("clear");
  const fs::path a = f.makeTemp();
  const fs::path b = f.makeTemp();
  const fs::path stored = f.store.addPhoto(f.makeTemp());
  f.store.clearTempPhotos();
  check(!fs::exists(a) && !fs::exists(b), "clearTempPhotos removes temp photos");
  check(fs::exists(f.store.absoluteImagePath(stored)),
        "clearTempPhotos keeps stored photos");
}

static void testPathConversion() {
  Fixture f("paths");
  const fs::path relative = fs::path("wardrobe") / "IMG_1.jpg";
  const fs::path absolute = f.store.absoluteImagePath(relative);
  check(absolute == f.dir / relative, "relative path becomes absolute");
  check(f.store.relativeImagePath(absolute) == relative,
        "absolute path becomes relative");
  check(f.store.absoluteImagePath(absolute) == absolute,
        "an absolute path stays the same");
  check(f.store.absoluteImagePath("").empty(), "empty path stays empty");
}

int main() {
  testNewTempPath();
  testAddPhoto();
  testAddPhotoKeepTemp();
  testAddMissingPhoto();
  testDiscardOnlyInsideTemp();
  testClearTempPhotos();
  testPathConversion();
  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
