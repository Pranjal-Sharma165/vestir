#include "ClothingRepository.h"
#include "TestHelpers.h"

#include <filesystem>
#include <fstream>

using Category = ClothingCategory::Value;
using SubCategory = ClothingCategory::SubCategory;
using Colour = ClothingColour::Value;
using Formality = OutfitFormality::Value;

struct Fixture {
  std::filesystem::path dir;
  Database database;
  PhotoStore photoStore;
  ClothingRepository repository;

  explicit Fixture(const std::string &name)
      : dir(std::filesystem::temp_directory_path() / ("vestir_test_" + name)),
        database(":memory:", readSchema()), photoStore(dir),
        repository(database, photoStore) {
    std::filesystem::remove_all(dir);
    database.initialize();
  }

  ~Fixture() { std::filesystem::remove_all(dir); }

  std::filesystem::path makePhoto() {
    const std::filesystem::path path = photoStore.newTempPath("jpg");
    std::ofstream(path) << "fake image";
    return path;
  }
};

template <typename Error, typename Function>
static bool throwsError(Function function) {
  try {
    function();
  } catch (const Error &) {
    return true;
  } catch (...) {
  }
  return false;
}

static void testListIsEmptyAtStart() {
  Fixture fixture("empty");
  check(fixture.repository.listStored().empty(),
        "new database has no clothing items");
}

static void testAddAndList() {
  Fixture fixture("add");
  const std::filesystem::path photo = fixture.makePhoto();
  ClothingItem item("Blue tee", Category::Top, SubCategory::TShirt,
                    Colour::Black, Formality::Casual, photo);

  const qint64 id = fixture.repository.add(item);
  check(id > 0, "add returns an id");

  const auto stored = fixture.repository.listStored();
  check(stored.size() == 1, "listStored returns the added item");
  if (stored.size() != 1)
    return;

  check(stored[0].id == id, "stored item has the returned id");
  check(stored[0].item->name() == "Blue tee", "stored item keeps its name");
  check(stored[0].item->category() == Category::Top, "stored item category");
  check(stored[0].item->subCategory() == SubCategory::TShirt,
        "stored item sub-category");
  check(stored[0].item->colour() == Colour::Black, "stored item colour");
  check(stored[0].item->formality() == Formality::Casual,
        "stored item formality");
  check(std::filesystem::exists(stored[0].item->m_imageUrl),
        "stored photo exists in the wardrobe folder");
  check(!std::filesystem::exists(photo), "temp photo is removed after add");
}

static void testAddWithMissingPhotoFails() {
  Fixture fixture("missing_photo");
  ClothingItem item("Ghost", Category::Top, SubCategory::Shirt, Colour::White,
                    Formality::Casual, fixture.dir / "nothing.jpg");

  check(fixture.repository.add(item) == 0, "add returns 0 without a photo");
  check(fixture.repository.listStored().empty(),
        "nothing is saved when the photo is missing");
}

static void testItemsAreListedInOrder() {
  Fixture fixture("order");
  ClothingItem first("First", Category::Top, SubCategory::Hoodie,
                     Colour::Gray, Formality::Casual, fixture.makePhoto());
  ClothingItem second("Second", Category::Bottom, SubCategory::Jeans,
                      Colour::Black, Formality::Casual, fixture.makePhoto());

  const qint64 firstId = fixture.repository.add(first);
  const qint64 secondId = fixture.repository.add(second);

  const auto stored = fixture.repository.listStored();
  check(stored.size() == 2, "both items are listed");
  if (stored.size() != 2)
    return;
  check(stored[0].id == firstId && stored[1].id == secondId,
        "items are listed in the order they were added");
}

static void testUpdateChangesLabels() {
  Fixture fixture("update");
  ClothingItem item("Tee", Category::Top, SubCategory::TShirt, Colour::Black,
                    Formality::Casual, fixture.makePhoto());
  const qint64 id = fixture.repository.add(item);

  ClothingRepository::Labels labels;
  labels.category = Category::Outerwear;
  labels.subCategory = SubCategory::Blazer;
  labels.colour = Colour::Gray;
  labels.formality = Formality::Formal;
  fixture.repository.update(id, labels);

  const auto stored = fixture.repository.listStored();
  check(stored.size() == 1, "item is still there after update");
  if (stored.size() != 1)
    return;
  check(stored[0].item->category() == Category::Outerwear,
        "update changes the category");
  check(stored[0].item->subCategory() == SubCategory::Blazer,
        "update changes the sub-category");
  check(stored[0].item->colour() == Colour::Gray, "update changes the colour");
  check(stored[0].item->formality() == Formality::Formal,
        "update changes the formality");
  check(stored[0].item->name() == "Tee", "update keeps the name");
}

static void testUpdateRejectsWrongSubCategory() {
  Fixture fixture("update_invalid");
  ClothingItem item("Tee", Category::Top, SubCategory::TShirt, Colour::Black,
                    Formality::Casual, fixture.makePhoto());
  const qint64 id = fixture.repository.add(item);

  ClothingRepository::Labels labels;
  labels.category = Category::Top;
  labels.subCategory = SubCategory::Jeans;
  labels.colour = Colour::Black;
  labels.formality = Formality::Casual;

  check(throwsError<InvalidInputError>(
            [&] { fixture.repository.update(id, labels); }),
        "update throws when sub-category is not in the category");
}

static void testUpdateUnknownIdThrows() {
  Fixture fixture("update_unknown");
  ClothingRepository::Labels labels;
  labels.category = Category::Top;
  labels.subCategory = SubCategory::Shirt;
  labels.colour = Colour::White;
  labels.formality = Formality::Casual;

  check(throwsError<NotFoundError>(
            [&] { fixture.repository.update(999, labels); }),
        "update throws for an unknown id");
}

static void testRemoveDeletesItemAndPhoto() {
  Fixture fixture("remove");
  ClothingItem item("Tee", Category::Top, SubCategory::TShirt, Colour::Black,
                    Formality::Casual, fixture.makePhoto());
  const qint64 id = fixture.repository.add(item);
  const std::filesystem::path storedPhoto =
      fixture.repository.listStored()[0].item->m_imageUrl;

  fixture.repository.remove(id);

  check(fixture.repository.listStored().empty(), "remove deletes the item");
  check(!std::filesystem::exists(storedPhoto), "remove deletes the photo file");
}

static void testRemoveUnknownIdThrows() {
  Fixture fixture("remove_unknown");
  check(throwsError<NotFoundError>([&] { fixture.repository.remove(999); }),
        "remove throws for an unknown id");
}

int main() {
  testListIsEmptyAtStart();
  testAddAndList();
  testAddWithMissingPhotoFails();
  testItemsAreListedInOrder();
  testUpdateChangesLabels();
  testUpdateRejectsWrongSubCategory();
  testUpdateUnknownIdThrows();
  testRemoveDeletesItemAndPhoto();
  testRemoveUnknownIdThrows();

  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  return failures == 0 ? 0 : 1;
}
