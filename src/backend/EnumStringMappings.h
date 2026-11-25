#ifndef ENUMSTRINGMAPPINGS_H
#define ENUMSTRINGMAPPINGS_H
/*
 * Contains mappings from the C++ enums to ML label values.
 */

#include "ClothingCategory.h"
#include "ClothingColour.h"
#include "OutfitFormality.h"
#include <array>

template <typename E> struct Label {
  E value;
  const char *text;
};

// Database labels. They must match the CHECK constraints in schema.sql.
constexpr std::array kCategories{
    Label<ClothingCategory::Value>{ClothingCategory::Value::Top, "Top"},
    Label<ClothingCategory::Value>{ClothingCategory::Value::Bottom, "Bottom"},
    Label<ClothingCategory::Value>{ClothingCategory::Value::Outerwear,
                                   "Outerwear"},
    Label<ClothingCategory::Value>{ClothingCategory::Value::Dress, "Dress"},
};

constexpr std::array kSubCategories{
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::TShirt,
                                         "t-shirt"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Blouse,
                                         "blouse"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Sweater,
                                         "sweater"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Hoodie,
                                         "hoodie"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Shirt,
                                         "shirt"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::TankTop,
                                         "tank top"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Jeans,
                                         "jeans"},
    Label<ClothingCategory::SubCategory>{
        ClothingCategory::SubCategory::Trousers, "trousers"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Shorts,
                                         "shorts"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Skirt,
                                         "skirt"},
    Label<ClothingCategory::SubCategory>{
        ClothingCategory::SubCategory::Leggings, "leggings"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Dress,
                                         "dress"},
    Label<ClothingCategory::SubCategory>{
        ClothingCategory::SubCategory::Jumpsuit, "jumpsuit"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Romper,
                                         "romper"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Jacket,
                                         "jacket"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Coat,
                                         "coat"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Blazer,
                                         "blazer"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Parka,
                                         "parka"},
    Label<ClothingCategory::SubCategory>{
        ClothingCategory::SubCategory::Cardigan, "cardigan"},
    Label<ClothingCategory::SubCategory>{ClothingCategory::SubCategory::Vest,
                                         "vest"},
};

constexpr std::array kColours{
    Label<ClothingColour::Value>{ClothingColour::Value::Black, "black"},
    Label<ClothingColour::Value>{ClothingColour::Value::White, "white"},
    Label<ClothingColour::Value>{ClothingColour::Value::Charcoal, "charcoal"},
    Label<ClothingColour::Value>{ClothingColour::Value::Gray, "gray"},
    Label<ClothingColour::Value>{ClothingColour::Value::Brown, "brown"},
    Label<ClothingColour::Value>{ClothingColour::Value::Beige, "beige"},
    Label<ClothingColour::Value>{ClothingColour::Value::Khaki, "khaki"},
    Label<ClothingColour::Value>{ClothingColour::Value::Cream, "cream"},
    Label<ClothingColour::Value>{ClothingColour::Value::NavyBlue, "navy blue"},
    Label<ClothingColour::Value>{ClothingColour::Value::Blue, "blue"},
    Label<ClothingColour::Value>{ClothingColour::Value::LightBlue,
                                 "light blue"},
    Label<ClothingColour::Value>{ClothingColour::Value::Green, "green"},
    Label<ClothingColour::Value>{ClothingColour::Value::OliveGreen,
                                 "olive green"},
    Label<ClothingColour::Value>{ClothingColour::Value::Red, "red"},
    Label<ClothingColour::Value>{ClothingColour::Value::Pink, "pink"},
    Label<ClothingColour::Value>{ClothingColour::Value::Purple, "purple"},
    Label<ClothingColour::Value>{ClothingColour::Value::Yellow, "yellow"},
    Label<ClothingColour::Value>{ClothingColour::Value::Orange, "orange"},
    Label<ClothingColour::Value>{ClothingColour::Value::Burgundy, "burgundy"},
};

constexpr std::array kFormalities{
    Label<OutfitFormality::Value>{OutfitFormality::Value::Casual, "casual"},
    Label<OutfitFormality::Value>{OutfitFormality::Value::SmartCasual,
                                  "smart-casual"},
    Label<OutfitFormality::Value>{OutfitFormality::Value::Formal, "formal"},
    Label<OutfitFormality::Value>{OutfitFormality::Value::Sporty, "sporty"},
};

/*!
 * \brief Get the enum value for a string.
 *
 * This string representation is used in storing values in the database, and in
 * communicating with Python functions.
 */
template <typename E, std::size_t N>
std::optional<E> valueFor(const std::array<Label<E>, N> &labels,
                          std::string_view text) {
  const auto it =
      std::find_if(labels.begin(), labels.end(), [text](const Label<E> &l) {
        return text == std::string_view{l.text};
      });
  return it != labels.end() ? std::optional<E>{it->value} : std::nullopt;
}

/*!
 * \brief Get the string representation of an enum value.
 *
 * This string representation is used in storing values in the database, and in
 * communicating with Python functions.
 */
template <typename E, std::size_t N>
std::optional<std::string_view> textFor(const std::array<Label<E>, N> &labels,
                                        E value) {
  const auto it =
      std::find_if(labels.begin(), labels.end(),
                   [value](const Label<E> &l) { return l.value == value; });
  return it != labels.end() ? std::optional<std::string_view>{it->text}
                            : std::nullopt;
}

#endif // ENUMSTRINGMAPPINGS_H
