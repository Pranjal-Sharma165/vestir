#include "OutfitRecommendation.h"

OutfitRecommendation::OutfitRecommendation(QObject *parent)
    : QObject(parent), m_title("Your outfit for the day"),
      m_eventTitle("Business lunch with <important person>"),
      m_clothingPieces({"<Piece 1>", "<Piece 2>", "<Piece 3>", "<Piece 4>"}),
      m_formality(OutfitFormality::Formal), m_weather(new WeatherSnapshot(this)) {
}