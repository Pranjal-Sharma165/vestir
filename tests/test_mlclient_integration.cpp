#include "ClothingCategory.h"
#include "EnumStringMappings.h"
#include "MlClient.h"
#include "TestHelpers.h"

const std::filesystem::path testImageDir = TEST_IMAGE_DIR;

// static void testRecommendation(MlClient &mlClient) {
//   mlClient.recommendLocal();
// }

struct ImageClassificationData {
  std::string imageName;
  bool isRecognizable;
  ClothingCategory::Value category;
  ClothingCategory::SubCategory subCategory;
  ClothingColour::Value colour;
  OutfitFormality::Value formality;
};

static void testPhotoClassification(MlClient &mlClient) {
  struct ImageClassificationData expected[] = {
      {
          .imageName = "jeans.jpg",
          .isRecognizable = true,
          .category = ClothingCategory::Value::Bottom,
          .subCategory = ClothingCategory::SubCategory::Jeans,
          .colour = ClothingColour::Value::Gray,
          .formality = OutfitFormality::Value::Casual,
      },
      {
          .imageName = "skirt.jpg",
          .isRecognizable = true,
          .category = ClothingCategory::Value::Bottom,
          .subCategory = ClothingCategory::SubCategory::Skirt,
          .colour = ClothingColour::Value::Charcoal,
          .formality = OutfitFormality::Value::Formal,
      },
      {
          .imageName = "sneakers.jpg",
          .isRecognizable = false,
      },
      {
          .imageName = "sweater.jpg",
          .isRecognizable = true,
          .category = ClothingCategory::Value::Top,
          .subCategory = ClothingCategory::SubCategory::Sweater,
          .colour = ClothingColour::Value::Gray,
          .formality = OutfitFormality::Value::Casual,
      },
      {
          .imageName = "tshirt.jpg",
          .isRecognizable = true,
          .category = ClothingCategory::Value::Top,
          .subCategory = ClothingCategory::SubCategory::TShirt,
          .colour = ClothingColour::Value::Brown,
          .formality = OutfitFormality::Value::Casual,
      },
  };

  for (auto &expectedOutput : expected) {
    auto labels =
        mlClient.classifyPhotoLocal(testImageDir / expectedOutput.imageName)
            .get();

    if (!expectedOutput.isRecognizable) {
      if (!labels.has_value()) {
        check(true, "Unclassifiable photo was not classified.");
      } else {
        check(false, "Unclassifiable photo was classified.");
      }
      return;
    }

    if (!labels.has_value()) {
      check(false, "Classifiable photo was not classified.");
      return;
    }

    auto values = labels.value();

    check(values.category == expectedOutput.category,
          std::format("Classifier category {} matches expected category "
                      "{} for image {}",
                      textFor(kCategories, values.category).value(),
                      textFor(kCategories, expectedOutput.category).value(),
                      expectedOutput.imageName));

    check(
        values.subCategory == expectedOutput.subCategory,
        std::format("Classifier sub-category {} matches expected sub-category "
                    "{} for image {}",
                    textFor(kSubCategories, values.subCategory).value(),
                    textFor(kSubCategories, expectedOutput.subCategory).value(),
                    expectedOutput.imageName));

    check(values.colour == expectedOutput.colour,
          std::format("Classifier colour {} matches expected colour "
                      "{} for image {}",
                      textFor(kColours, values.colour).value(),
                      textFor(kColours, expectedOutput.colour).value(),
                      expectedOutput.imageName));

    check(values.formality == expectedOutput.formality,
          std::format("Classifier formality {} matches expected formality "
                      "{} for image {}",
                      textFor(kFormalities, values.formality).value(),
                      textFor(kFormalities, expectedOutput.formality).value(),
                      expectedOutput.imageName));
  }
}

// static void testEventClassification(MlClient &mlClient) {
//   mlClient.classifyEventsLocal();
// }

int main() {
  MlClient mlClient;
  testPhotoClassification(mlClient);
  std::cout << (failures == 0 ? "All tests passed\n" : "Some tests failed\n");
  // return failures == 0 ? 0 : 1;
}
