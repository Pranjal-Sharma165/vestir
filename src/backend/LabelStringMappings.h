/*
 * String to enum value mappings for the labels of clothing items.
 *
 * To be used for reading and writing values in the database (if there is time,
 * right now it looks like we are using the hardcoded values in
 * EnumStringMappings.h).
 *
 * NOT to be used for user-facing strings. Refer to each of the enum types for
 * methods to get display strings.
 */

#ifndef LABELSTRINGMAPPINGS_H
#define LABELSTRINGMAPPINGS_H

#include "ClothingCategory.h"
#include "ClothingColour.h"
#include "OutfitFormality.h"
#include <string_view>
#include <unordered_map>

#define DEFINE_CATEGORY_STRING_MAP(symbol, string)                             \
  {std::string_view(#symbol), ClothingCategory::Value::symbol},
static const std::unordered_map<std::string_view, ClothingCategory::Value>
    categoryStringToValue = {CATEGORY_LIST(DEFINE_CATEGORY_STRING_MAP)};
#undef DEFINE_CATEGORY_STRING_MAP
#define DEFINE_CATEGORY_STRING_REV_MAP(symbol, string)                         \
  {ClothingCategory::Value::symbol, std::string_view(#symbol)},
static const std::unordered_map<ClothingCategory::Value, std::string_view>
    categoryValueToString = {CATEGORY_LIST(DEFINE_CATEGORY_STRING_REV_MAP)};
#undef DEFINE_CATEGORY_STRING_REV_MAP

#define DEFINE_SUBCATEGORY_STRING_MAP(child, parent)                           \
  {std::string_view(#child), ClothingCategory::SubCategory::child},
static const std::unordered_map<std::string_view, ClothingCategory::SubCategory>
    subCategoryStringToValue = {
        SUBCATEGORY_LIST(DEFINE_SUBCATEGORY_STRING_MAP)};
#undef DEFINE_SUBCATEGORY_STRING_MAP
#define DEFINE_SUBCATEGORY_STRING_REV_MAP(child, parent)                       \
  {ClothingCategory::SubCategory::child, std::string_view(#child)},
static const std::unordered_map<ClothingCategory::SubCategory, std::string_view>
    subCategoryValueToString = {
        SUBCATEGORY_LIST(DEFINE_SUBCATEGORY_STRING_REV_MAP)};
#undef DEFINE_SUBCATEGORY_STRING_REV_MAP

#define DEFINE_COLOUR_STRING_MAP(colour)                                       \
  {std::string_view(#colour), ClothingColour::Value::colour},
static const std::unordered_map<std::string_view, ClothingColour::Value>
    colourStringToValue = {COLOUR_LIST(DEFINE_COLOUR_STRING_MAP)};
#undef DEFINE_COLOUR_STRING_MAP
#define DEFINE_COLOUR_STRING_REV_MAP(colour)                                   \
  {ClothingColour::Value::colour, std::string_view(#colour)},
static const std::unordered_map<ClothingColour::Value, std::string_view>
    colourValueToString = {COLOUR_LIST(DEFINE_COLOUR_STRING_REV_MAP)};
#undef DEFINE_COLOUR_STRING_REV_MAP

#define DEFINE_FORMALITY_STRING_MAP(formality, string)                         \
  {std::string_view(#formality), OutfitFormality::Value::formality},
static const std::unordered_map<std::string_view, OutfitFormality::Value>
    formalityStringToValue = {FORMALITY_LIST(DEFINE_FORMALITY_STRING_MAP)};
#undef DEFINE_FORMALITY_STRING_MAP
#define DEFINE_FORMALITY_STRING_REV_MAP(formality, string)                     \
  {OutfitFormality::Value::formality, std::string_view(#formality)},
static const std::unordered_map<OutfitFormality::Value, std::string_view>
    formalityValueToString = {FORMALITY_LIST(DEFINE_FORMALITY_STRING_REV_MAP)};
#undef DEFINE_FORMALITY_STRING_REV_MAP

#endif // LABELSTRINGMAPPINGS_H
