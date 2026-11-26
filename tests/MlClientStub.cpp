#include "MlClient.h"

#include <stdexcept>

// Stand-in used only by the unit tests so they never start Python.
MlClient::MlClient(QObject *parent) : QObject(parent) {}

std::future<std::vector<OutfitFormality::Value>>
MlClient::classifyEventsLocal(const std::vector<std::string>) {
  throw std::logic_error("MlClient is not available in unit tests");
}

std::future<std::vector<OutfitRecommendation>> MlClient::recommendLocal(
    const std::vector<ClothingRepository::StoredItem> &,
    std::vector<OutfitRecommendation>, int, std::optional<WeatherData>,
    std::optional<CalendarEntry>) {
  throw std::logic_error("MlClient is not available in unit tests");
}
